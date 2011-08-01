#ifndef __MESH_H__
#define __MESH_H__
#include <3DIO/3DIO.h>

///Generic Mesh container
struct Mesh {
	///Raw pointer to generic tdio_library::Object (But we only use OBJs for now)
	tdio_library::Object<tdio_library::GEN> *object;
	
	///Num. faces
	int nFace;

	///Num. vertices
	int nVtx;

	///Vertices
	tdio_library::vector3 *vtx;

	///Faces
	tdio_library::Face *faces;

	///Various metrics
	tdio_library::vector3 min,max,center,range;
	
	
	///Primary axis defines the axis of greatest range.
	int primaryAxis;

	///Height is the limiting metric in MineCraft, 128.0 units is the max.
	float heightLimit;
};

#endif //__MESH_H__