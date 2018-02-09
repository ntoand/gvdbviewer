#ifndef SPARSE_VOL_H
#define SPARSE_VOL_H

// GVDB library
#include "gvdb.h"			
using namespace nvdb;


struct nvVertex {
	nvVertex(float x1, float y1, float z1, float tx1, float ty1, float tz1) { x=x1; y=y1; z=z1; tx=tx1; ty=ty1; tz=tz1; }
	float	x, y, z;
	float	nx, ny, nz;
	float	tx, ty, tz;
};
struct nvFace {
	nvFace(unsigned int x1, unsigned int y1, unsigned int z1) { a=x1; b=y1; c=z1; }
	unsigned int  a, b, c;
};

class SparseVol {

private:
	VolumeGVDB	gvdb;

	// control
	bool _initialized;
	unsigned int _tex;
	int _width, _height;
	unsigned int m_screenquad_prog;
	unsigned int m_screenquad_utex, m_screenquad_uscreen, m_screenquad_ucoords;
	unsigned int m_screenquad_vbo[2];


private: 
	void initScreenQuadGL();
	void renderScreenQuadGL ();
	void setup();

public:
	SparseVol();
	~SparseVol();
	void init(int w, int h);
	void render(const float pos[3]);

};

#endif