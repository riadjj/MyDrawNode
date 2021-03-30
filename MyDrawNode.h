#ifndef _MYDRAWNODE_H_
#define _MYDRAWNODE_H_

#include "cocos2d.h"

USING_NS_CC;

class CDrawNode : public DrawNode {
public:
	CDrawNode(){};
	CDrawNode(int lineWidth);

public:
	void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override;
	void onDraw(const Mat4 &transform, uint32_t flags) override;
	void drawSolidPolyFixed(const Vec2 *verts, int count, const Color4F *fillColor);

public:
	Vec2 *_vertices;
    
    int delay = 0;

};

//////////////////////////////////////////////////////////////////

class TrailEffect : public CDrawNode {
public:
	TrailEffect(int lineWidth);

	static TrailEffect *create(float x, float y, int length = 40);
	static TrailEffect *create(const Vec2 &pos, int length = 40);

public:
	void initialize();
	void updateTrail(float sx, float sy);
	void drawTrail(int wi, int wo, const Color4F &ci, const Color4F &co);

public:
	std::vector<Vec2> _trailPos;
	int _length = 0;
};

//////////////////////////////////////////////////////////////////
//영역 테스트용 테스트드로우
class TestDraw : public CDrawNode {
public:
    TestDraw();
    
    static TestDraw* create(const Vec2& start, const Vec2& end);
    void redraw(const Vec2& start, const Vec2& end);
    
    void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override;
    void onDraw(const Mat4 &transform, uint32_t flags) override;
    
    Vec2 _start;
    Vec2 _end;
    
    DrawNode *drawNode;
};

//////////////////////////////////////////////////////////////////
//메인화면 임시 낚시줄
class LineDraw : public CDrawNode {
public:
    LineDraw();
    
    static LineDraw* create(const Vec2& start, const Vec2& end);
    void redraw(const Vec2& start, const Vec2& end);
    
    void draw(Renderer *renderer, const Mat4 &transform, uint32_t flags) override;
    void onDraw(const Mat4 &transform, uint32_t flags) override;
    
    Vec2 _start;
    Vec2 _end;
    
    DrawNode *drawNode;
};

#endif
