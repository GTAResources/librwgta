#ifndef _COLLISION_H_
#define _COLLISION_H_

struct CColSphere
{
	CVector center;
	float radius;
	uint8 surface;
	uint8 piece;

	void Set(float radius, const CVector &center, uint8 surf, uint8 piece);
};

struct CColBox
{
	CVector min;
	CVector max;
	uint8 surface;
	uint8 piece;

	void Set(const CVector &min, const CVector &max, uint8 surf, uint8 piece);
};

struct CColLine
{
	CVector p0;
	int pad0;
	CVector p1;
	int pad1;

	void Set(const CVector &p0, const CVector &p1);
};

struct CColTriangle
{
	uint16 a;
	uint16 b;
	uint16 c;
	uint8 surface;

	void Set(int a, int b, int c, uint8 surf);
};

struct CColTrianglePlane
{
	CVector normal;
	float dist;
	uint8 dir;

	void Set(const CVector *v, CColTriangle &tri);
	void GetNormal(CVector &n) const { n = normal; }
	float CalcPoint(const CVector &v) const { return DotProduct(normal, v) - dist; };
};

//struct CColPoint
//{
//	rw::V3d point;
//	int pad1;
//	rw::V3d normal;
//	int pad2;
//	rw::uint8 surfaceTypeA;
//	rw::uint8 pieceTypeA;
//	rw::uint8 pieceTypeB;
//	rw::uint8 surfaceTypeB;
//	float depth;
//};

struct CColModel
{
	CColSphere boundingSphere;
	CColBox boundingBox;
	short numSpheres;
	short numLines;
	short numBoxes;
	short numTriangles;
	int level;
	//rw::uint8 unk1;
	CColSphere *spheres;
	CColLine *lines;
	CColBox *boxes;
	CVector *vertices;
	CColTriangle *triangles;
	//int unk2;

	CColModel(void);
	~CColModel(void);
};

class CCollision
{
public:
	static eLevelName ms_collisionInMemory;

	static void Update(void);
	static void LoadCollisionWhenINeedIt(bool changeLevel);
	static void DrawColModel(const CMatrix &mat, const CColModel &colModel);

	// all these return true if there's a collision
	static bool TestSphereSphere(const CColSphere &s1, const CColSphere &s2);
	static bool TestSphereBox(const CColSphere &sph, const CColBox &box);
	static bool TestLineBox(const CColLine &line, const CColBox &box);
	static bool TestVerticalLineBox(const CColLine &line, const CColBox &box);
	static bool TestLineTriangle(const CColLine &line, const CVector *verts, const CColTriangle &tri, const CColTrianglePlane &plane);
	static bool TestLineSphere(const CColLine &line, const CColSphere &sph);
	// LineOfSight
	// SphereTriangle

	static float DistToLine(const CVector *l0, const CVector *l1, const CVector *point);
};

#endif
