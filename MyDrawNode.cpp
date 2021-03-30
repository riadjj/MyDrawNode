#include "MyDrawNode.h"

static Vec2 v2fzero(0.0f, 0.0f);

static inline Vec2 v2f(float x, float y)
{
	Vec2 ret(x, y);
	return ret;
}

static inline Vec2 v2fadd(const Vec2 &v0, const Vec2 &v1)
{
	return v2f(v0.x + v1.x, v0.y + v1.y);
}

static inline Vec2 v2fsub(const Vec2 &v0, const Vec2 &v1)
{
	return v2f(v0.x - v1.x, v0.y - v1.y);
}

static inline Vec2 v2fmult(const Vec2 &v, float s)
{
	return v2f(v.x * s, v.y * s);
}

static inline Vec2 v2fperp(const Vec2 &p0)
{
	return v2f(-p0.y, p0.x);
}

static inline Vec2 v2fneg(const Vec2 &p0)
{
	return v2f(-p0.x, -p0.y);
}

static inline float v2fdot(const Vec2 &p0, const Vec2 &p1)
{
	return  p0.x * p1.x + p0.y * p1.y;
}

static inline Vec2 v2fforangle(float _a_)
{
	return v2f(cosf(_a_), sinf(_a_));
}

static inline Vec2 v2fnormalize(const Vec2 &p)
{
	Vec2 r(p.x, p.y);
	r.normalize();
	return v2f(r.x, r.y);
}

static inline Vec2 __v2f(const Vec2 &v)
{
	//#ifdef __LP64__
	return v2f(v.x, v.y);
	// #else
	//     return * ((Vec2*) &v);
	// #endif
}

static inline Tex2F __t(const Vec2 &v)
{
	return *(Tex2F*)&v;
}

float Clamp(float value, float min, float max)
{
	return value > max ? max : value <= min ? min : value;
}

CDrawNode::CDrawNode(int lineWidth)
{
	_vao = 0;
	_vbo = 0;
	_vaoGLPoint = 0;
	_vboGLPoint = 0;
	_vaoGLLine = 0;
	_vboGLLine = 0;
	_bufferCapacity = 0;
	_bufferCount = 0;
	_buffer = nullptr;
	_bufferCapacityGLPoint = 0;
	_bufferCountGLPoint = 0;
	_bufferGLPoint = nullptr;
	_bufferCapacityGLLine = 0;
	_bufferCountGLLine = 0;
	_bufferGLLine = nullptr;
	_dirty = false;
	_dirtyGLPoint = false;
	_dirtyGLLine = false;
	_lineWidth = lineWidth;
	_defaultLineWidth = lineWidth;
	_blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
}

void CDrawNode::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
	if (_bufferCount)
	{
		_customCommand.init(_globalZOrder, transform, flags);
		_customCommand.func = CC_CALLBACK_0(CDrawNode::onDraw, this, transform, flags);
		renderer->addCommand(&_customCommand);
	}

	if (_bufferCountGLPoint)
	{
		_customCommandGLPoint.init(_globalZOrder, transform, flags);
		_customCommandGLPoint.func = CC_CALLBACK_0(DrawNode::onDrawGLPoint, this, transform, flags);
		renderer->addCommand(&_customCommandGLPoint);
	}

	if (_bufferCountGLLine)
	{
		_customCommandGLLine.init(_globalZOrder, transform, flags);
		_customCommandGLLine.func = CC_CALLBACK_0(DrawNode::onDrawGLLine, this, transform, flags);
		renderer->addCommand(&_customCommandGLLine);
	}
}

void CDrawNode::onDraw(const Mat4 &transform, uint32_t flags)
{
	auto glProgram = getGLProgram();
	glProgram->use();
	glProgram->setUniformsForBuiltins(transform);

	GL::blendFunc(_blendFunc.src, _blendFunc.dst);

	if (_dirty)
	{
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(V2F_C4B_T2F)*_bufferCapacity, _buffer, GL_STREAM_DRAW);

		_dirty = false;
        delay = 0;
	}
	if (Configuration::getInstance()->supportsShareableVAO())
	{
		GL::bindVAO(_vao);
        
        if (delay++ > 30) {
            delay = 0;
            this->clear();
        }
	}
	else
	{
		GL::enableVertexAttribs(GL::VERTEX_ATTRIB_FLAG_POS_COLOR_TEX);

		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		// vertex
		glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(V2F_C4B_T2F), (GLvoid *)offsetof(V2F_C4B_T2F, vertices));
		// color
		glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(V2F_C4B_T2F), (GLvoid *)offsetof(V2F_C4B_T2F, colors));
		// texcood
		glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_TEX_COORD, 2, GL_FLOAT, GL_FALSE, sizeof(V2F_C4B_T2F), (GLvoid *)offsetof(V2F_C4B_T2F, texCoords));
	}
	glDrawArrays(GL_TRIANGLE_STRIP, 0, _bufferCount);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (Configuration::getInstance()->supportsShareableVAO())
	{
		GL::bindVAO(0);
	}

	CC_INCREMENT_GL_DRAWN_BATCHES_AND_VERTICES(1, _bufferCount);
	CHECK_GL_ERROR_DEBUG();
}

void CDrawNode::drawSolidPolyFixed(const Vec2 *verts, int count, const Color4F *fillColor)
{
	CCASSERT(count >= 0, "invalid count value");

	auto  triangle_count = (count - 2);
	auto vertex_count = 3 * triangle_count;
	ensureCapacity(vertex_count);

	V2F_C4B_T2F_Triangle *triangles = (V2F_C4B_T2F_Triangle *)(_buffer + _bufferCount);
	V2F_C4B_T2F_Triangle *cursor = triangles;

	for (int i = 0; i < count - 2; i++)
	{
		V2F_C4B_T2F_Triangle tmp = {
			{ verts[i + 0], Color4B(fillColor[i + 0]), __t(v2fzero) },
			{ verts[i + 1], Color4B(fillColor[i + 1]), __t(v2fzero) },
			{ verts[i + 2], Color4B(fillColor[i + 2]), __t(v2fzero) },
		};

		*cursor++ = tmp;
	}

	_bufferCount += vertex_count;

	_dirty = true;
}

//////////////////////////////////////////////////////////////////

TrailEffect::TrailEffect(int lineWidth)
{
	_vao = 0;
	_vbo = 0;
	_vaoGLPoint = 0;
	_vboGLPoint = 0;
	_vaoGLLine = 0;
	_vboGLLine = 0;
	_bufferCapacity = 0;
	_bufferCount = 0;
	_buffer = nullptr;
	_bufferCapacityGLPoint = 0;
	_bufferCountGLPoint = 0;
	_bufferGLPoint = nullptr;
	_bufferCapacityGLLine = 0;
	_bufferCountGLLine = 0;
	_bufferGLLine = nullptr;
	_dirty = false;
	_dirtyGLPoint = false;
	_dirtyGLLine = false;
	_lineWidth = lineWidth;
	_defaultLineWidth = lineWidth;
	_blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
}

TrailEffect *TrailEffect::create(float x, float y, int length)
{
	auto ret = new TrailEffect(2);
	if (ret && ret->init())
	{
		ret->autorelease();

		ret->_length = length;
		ret->setPosition(x, y);

		ret->initialize();
	}
	else
	{
		CC_SAFE_DELETE(ret);
	}

	return ret;
}

TrailEffect *TrailEffect::create(const Vec2 &pos, int length)
{
	auto ret = new TrailEffect(2);
	if (ret && ret->init())
	{
		ret->autorelease();

		ret->_length = length;
		ret->setPosition(pos);

		ret->initialize();
	}
	else
	{
		CC_SAFE_DELETE(ret);
	}

	return ret;
}

void TrailEffect::initialize()
{
	for (int i = 0; i < _length; i++) {
		_trailPos.push_back(this->getPosition());
	}
}

//처음 두께, 마지막 두께, 처음 색, 마지막 색
void TrailEffect::drawTrail(int wi, int wo, const Color4F &ci, const Color4F &co)
{
	this->clear();

	int lineWidth = wi;
	float lineDir = 0;
	Vec2 lineVec;
	Color4F color = ci;

	int trailEnd = _trailPos.size() - 1;
	int vertLength = _trailPos.size() * 2;

	Vec2 *vertices = new Vec2[vertLength - 4];
	Color4F *colors = new Color4F[vertLength - 4];

	if ((int)_trailPos.size() > 2) {
		for (int i = 0, j = 0; i < trailEnd - 1; i++, j += 2) {
			lineWidth = wi * (trailEnd - i) / trailEnd + wo * (i + 1) / trailEnd;
			color.a = Clamp(ci.a * (float)(trailEnd - i) / (float)trailEnd + co.a * (float)(i + 1) / (float)trailEnd, 0.0f, 1.0f);
			color.r = Clamp((ci.r * ((trailEnd - i) / (float)trailEnd) + co.r * (1 - ((trailEnd - i) / (float)trailEnd))), 0.0f, 1.0f);
			color.g = Clamp((ci.g * ((trailEnd - i) / (float)trailEnd) + co.g * (1 - ((trailEnd - i) / (float)trailEnd))), 0.0f, 1.0f);
			color.b = Clamp((ci.b * ((trailEnd - i) / (float)trailEnd) + co.b * (1 - ((trailEnd - i) / (float)trailEnd))), 0.0f, 1.0f);

			lineDir = 180 - CC_RADIANS_TO_DEGREES(atan2(_trailPos[i + 1].y - _trailPos[i].y, _trailPos[i + 1].x - _trailPos[i].x));
			lineVec = this->getParent()->convertToNodeSpaceAR(_trailPos[i]);
			float x1 = lineVec.x + cos(CC_DEGREES_TO_RADIANS(lineDir + 90)) * lineWidth;
			float x2 = lineVec.x + cos(CC_DEGREES_TO_RADIANS(lineDir - 90)) * lineWidth;
			float y1 = lineVec.y + sin(CC_DEGREES_TO_RADIANS(-(lineDir + 90))) * lineWidth;
			float y2 = lineVec.y + sin(CC_DEGREES_TO_RADIANS(-(lineDir - 90))) * lineWidth;

			vertices[j + 0] = Vec2(x1, y1);
			vertices[j + 1] = Vec2(x2, y2);

			colors[j + 0] = color;
			colors[j + 1] = color;
		}
	}

	this->drawSolidPolyFixed(vertices, vertLength - 4, colors);
	delete[] vertices;
	delete[] colors;
}

void TrailEffect::updateTrail(float sx, float sy)
{
	for (int i = _trailPos.size() - 1; i > 0; i--) {
		_trailPos[i] = _trailPos[i - 1];
	}

	_trailPos[0].x = sx;
	_trailPos[0].y = sy;
}

//////////////////////////////////////////////////////////////////

TestDraw::TestDraw()
{
    _vao = 0;
    _vbo = 0;
    _vaoGLPoint = 0;
    _vboGLPoint = 0;
    _vaoGLLine = 0;
    _vboGLLine = 0;
    _bufferCapacity = 0;
    _bufferCount = 0;
    _buffer = nullptr;
    _bufferCapacityGLPoint = 0;
    _bufferCountGLPoint = 0;
    _bufferGLPoint = nullptr;
    _bufferCapacityGLLine = 0;
    _bufferCountGLLine = 0;
    _bufferGLLine = nullptr;
    _dirty = false;
    _dirtyGLPoint = false;
    _dirtyGLLine = false;
    _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    
    drawNode = DrawNode::create();
    this->addChild(drawNode);
}

TestDraw *TestDraw::create(const Vec2& start, const Vec2& end)
{
    auto ret = new TestDraw();
    if (ret && ret->init())
    {
        ret->autorelease();
        ret->_start = start;
        ret->_end = end;
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }
    
    return ret;
}

void TestDraw::redraw(const Vec2& start, const Vec2& end)
{
    _start = start;
    _end = end;
}

void TestDraw::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    _customCommand.init(_globalZOrder, transform, flags);
    _customCommand.func = CC_CALLBACK_0(TestDraw::onDraw, this, transform, flags);
    renderer->addCommand(&_customCommand);
}

void TestDraw::onDraw(const Mat4 &transform, uint32_t flags)
{
    drawNode->clear();
    Color4F color4f = Color4F::RED;
    
    drawNode->setLineWidth(5.0);
    drawNode->drawRect(_start, _end, color4f);
    
//    drawNode->drawSolidRect(_start, _end, color4f);
}

//////////////////

LineDraw::LineDraw()
{
    _vao = 0;
    _vbo = 0;
    _vaoGLPoint = 0;
    _vboGLPoint = 0;
    _vaoGLLine = 0;
    _vboGLLine = 0;
    _bufferCapacity = 0;
    _bufferCount = 0;
    _buffer = nullptr;
    _bufferCapacityGLPoint = 0;
    _bufferCountGLPoint = 0;
    _bufferGLPoint = nullptr;
    _bufferCapacityGLLine = 0;
    _bufferCountGLLine = 0;
    _bufferGLLine = nullptr;
    _dirty = false;
    _dirtyGLPoint = false;
    _dirtyGLLine = false;
    _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    
    drawNode = DrawNode::create();
    this->addChild(drawNode);
}

LineDraw *LineDraw::create(const Vec2& start, const Vec2& end)
{
    auto ret = new LineDraw();
    if (ret && ret->init())
    {
        ret->autorelease();
        ret->_start = start;
        ret->_end = end;
    }
    else
    {
        CC_SAFE_DELETE(ret);
    }
    return ret;
}

void LineDraw::redraw(const Vec2& start, const Vec2& end)
{
    _start = start;
    _end = end;
}

void LineDraw::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    _customCommand.init(_globalZOrder, transform, flags);
    _customCommand.func = CC_CALLBACK_0(LineDraw::onDraw, this, transform, flags);
    renderer->addCommand(&_customCommand);
}

void LineDraw::onDraw(const Mat4 &transform, uint32_t flags)
{
    drawNode->clear();
    Color4F color4f = Color4F::BLACK;
    
    drawNode->setLineWidth(1.0);
    drawNode->drawLine(_start, _end, color4f);
}

