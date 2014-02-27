 cSourceCL =
 "#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable \n  \n typedef struct { \n 	float r, g, b; \n } cu_color; \n  \n typedef struct { \n 	float x, y, r, I; \n 	int type; \n 	unsigned short maxx, minx; \n 	cu_color e; \n 	unsigned long posInFragBuf; \n } cu_particle_splotch; \n  \n typedef struct { \n 	float p[12]; \n 	bool projection; \n 	int xres, yres; \n 	float fovfct, dist, xfac; \n 	float minrad_pix; \n 	int ptypes; \n 	float zmaxval, zminval; \n 	bool col_vector[8]; \n 	float brightness[8]; \n 	float rfac; \n } cu_param; \n  \n typedef struct { \n 	float val; \n 	cu_color color; \n } cu_color_map_entry; \n  \n typedef struct { \n 	cu_color_map_entry *map; \n 	int mapSize; \n 	int *ptype_points; \n 	int ptypes; \n } cu_colormap_info; \n  \n typedef struct { \n 	float aR, aG, aB; \n } cu_fragment_AeqE; \n  \n typedef struct { \n 	float aR, aG, aB; \n 	float qR, qG, qB; \n } cu_fragment_AneqE; \n  \n cu_color get_color(int ptype, float val, int mapSize, int ptypes,__global cu_color_map_entry *dmap_in,__global int *ptype_points_in) \n { \n 	int map_size; \n 	int map_ptypes; \n  \n 	map_size = mapSize; \n 	map_ptypes = ptypes; \n  \n 	int start, end; \n 	start =ptype_points_in[ptype]; \n 	if ( ptype == map_ptypes-1) end =map_size-1; \n 	else	end =ptype_points_in[ptype+1]-1; \n  \n 	int i=start; \n 	while ((val>dmap_in[i+1].val) && (i<end)) ++i; \n  \n 	const float fract = (val-dmap_in[i].val)/(dmap_in[i+1].val-dmap_in[i].val); \n 	cu_color clr1=dmap_in[i].color, clr2=dmap_in[i+1].color; \n 	cu_color clr; \n 	clr.r =clr1.r + fract*(clr2.r-clr1.r); \n 	clr.g =clr1.g + fract*(clr2.g-clr1.g); \n 	clr.b =clr1.b + fract*(clr2.b-clr1.b); \n  \n 	return clr; \n } \n  \n __kernel void k_render1 \n (__global cu_particle_splotch *p, int nP, __global \n 		cu_fragment_AeqE *buf, float grayabsorb, int mapSize, int types,__global cu_param *dparams1, \n 		__global cu_color_map_entry *dmap_in,__global int *ptype_points_in) \n { \n  \n 	int m; \n 	m = get_global_id(0); \n 	if (m<nP)  \n         { \n 		int ptype = p[m].type; \n 		float col1=p[m].e.r,col2=p[m].e.g,col3=p[m].e.b; \n 		clamp (0.0000001,0.9999999,col1); \n 		if (dparams1->col_vector[ptype]) \n 		{ \n 			col2 = clamp (0.0000001,0.9999999,col2); \n 			col3 = clamp (0.0000001,0.9999999,col3); \n 		} \n 		float intensity=p[m].I; \n 		intensity = clamp (0.0000001,0.9999999,intensity); \n 		intensity *= dparams1->brightness[ptype]; \n  \n 		cu_color e; \n 		if (dparams1->col_vector[ptype]) \n 		{ \n 			e.r=col1*intensity; \n 			e.g=col2*intensity; \n 			e.b=col3*intensity; \n 		} \n 		else \n 		{ \n 			if (ptype<types) \n 			{ \n 				e = get_color(ptype, col1, mapSize, types,dmap_in,ptype_points_in); \n 				e.r *= intensity; \n 				e.g *= intensity; \n 				e.b *= intensity; \n 			} \n 			else \n 			{	e.r =e.g =e.b =0.0;} \n 		} \n  \n 		const float sigma0 =0.584287628; \n 		float r = p[m].r; \n 		const float radsq = 2.25*r*r; \n 		const float stp = -0.5/(r*r*sigma0*sigma0); \n 		const float powtmp = pow(3.14159265358979323846264338327950288,1./3.); \n 		const float intens = -0.5/(2*sqrt(3.14159265358979323846264338327950288)*powtmp); \n 		e.r*=intens; e.g*=intens; e.b*=intens; \n  \n 		const float posx=p[m].x, posy=p[m].y; \n 		unsigned int fpos =p[m].posInFragBuf; \n 		 \n 		int minx=p[m].minx; \n 		int maxx=p[m].maxx; \n  \n 		const float rfacr=dparams1->rfac*r;		 \n 		int miny=(posy-rfacr+1); \n 		miny=max(miny,0); \n 		int maxy=(posy+rfacr+1); \n 		maxy = min(dparams1->yres, maxy); \n  \n 		for (int x=minx; x<maxx; ++x) \n 		{ \n 			float dxsq=(x-posx)*(x-posx); \n 			for (int y=miny; y<maxy; ++y) \n 			{ \n 				float dsq = (y-posy)*(y-posy) + dxsq; \n 				if (dsq<radsq) \n 				{ \n 					float att = pow((float)2.71828,(stp*dsq)); \n 					buf[fpos].aR = att*e.r; \n 					buf[fpos].aG = att*e.g; \n 					buf[fpos].aB = att*e.b; \n 				} \n 				else \n 				{ \n 					buf[fpos].aR =0.0; \n 					buf[fpos].aG =0.0; \n 					buf[fpos].aB =0.0; \n 				} \n  \n 				fpos++; \n 			} \n 		} \n 	} \n } \n  \n   \n __kernel void k_render2 \n (__global cu_particle_splotch *p, int nP,__global \n cu_fragment_AneqE *buf, float grayabsorb, int mapSize, int types,__global cu_param *dparams1,__global cu_color_map_entry *dmap_in,__global int *ptype_points_in) \n { \n  \n 	int m; \n 	m = get_global_id(0); \n 	if (m<nP)  \n 	{ \n 		int ptype = p[m].type; \n 		float col1=p[m].e.r,col2=p[m].e.g,col3=p[m].e.b; \n 		clamp (0.0000001,0.9999999,col1); \n 		if (dparams1->col_vector[ptype]) \n 		{ \n 			col2 = clamp (0.0000001,0.9999999,col2); \n 			col3 = clamp (0.0000001,0.9999999,col3); \n 		} \n 		float intensity=p[m].I; \n 		intensity = clamp (0.0000001,0.9999999,intensity); \n 		intensity *= dparams1->brightness[ptype]; \n  \n 		cu_color e; \n 		if (dparams1->col_vector[ptype]) \n 		{ \n 			e.r=col1*intensity; \n 			e.g=col2*intensity; \n 			e.b=col3*intensity; \n 		} \n 		else \n 		{ \n 			if (ptype<types) \n 			{ \n 				e = get_color(ptype, col1, mapSize, types,dmap_in,ptype_points_in); \n 				e.r *= intensity; \n 				e.g *= intensity; \n 				e.b *= intensity; \n 			} \n 			else \n 			{	e.r =e.g =e.b =0.0f;} \n 		} \n  \n 		const float powtmp = pow(3.14159265358979323846264338327950288,1./3.); \n 		const float sigma0 = powtmp/sqrt(2*3.14159265358979323846264338327950288); \n  \n 		 float r = p[m].r; \n 		const float radsq = 2.25f*r*r; \n 		const float stp = -0.5/(r*r*sigma0*sigma0); \n  \n 		cu_color q; \n  \n 		q.r = e.r/(e.r+grayabsorb); \n 		q.g = e.g/(e.g+grayabsorb); \n 		q.b = e.b/(e.b+grayabsorb); \n  \n 		const float intens = -0.5/(2*sqrt(3.14159265358979323846264338327950288)*powtmp); \n 		e.r*=intens; e.g*=intens; e.b*=intens; \n  \n 		const float posx=p[m].x, posy=p[m].y; \n 		unsigned int fpos =p[m].posInFragBuf; \n  \n 		int minx=p[m].minx; \n 		int maxx=p[m].maxx; \n  \n 		const float rfacr=dparams1->rfac*r;		 \n 		int miny=(int)(posy-rfacr+1); \n 		miny=max(miny,0); \n 		int maxy=(int)(posy+rfacr+1); \n                 maxy = min(dparams1->yres, maxy); \n  \n 		for (int x=minx; x<maxx; ++x) \n 		{ \n 			float dxsq=(x-posx)*(x-posx); \n 			for (int y=miny; y<maxy; ++y) \n 			{ \n 				float dsq = (y-posy)*(y-posy) + dxsq; \n 				if (dsq<radsq) \n 				{ \n 					float att =pow((float)2.71828,(stp*dsq)); \n 					float expm1 = pow((float)2.71828,(att*e.r))-1.0; \n  \n 					buf[fpos].aR = expm1; \n 					buf[fpos].qR = q.r; \n 					expm1= pow((float)2.71828,(att*e.g))-1.0; \n 					buf[fpos].aG = expm1; \n 					buf[fpos].qG = q.g; \n 					expm1= pow((float)2.71828,(att*e.b))-1.0; \n 					buf[fpos].aB = expm1; \n 					buf[fpos].qB = q.b; \n 				} \n 				else \n 				{ \n 					buf[fpos].aR =0.0; \n 					buf[fpos].aG =0.0; \n 					buf[fpos].aB =0.0; \n 					buf[fpos].qR =1.0; \n 					buf[fpos].qG =1.0; \n 					buf[fpos].qB =1.0; \n 				} \n  \n 				fpos++; \n 			} //y \n 		} //x \n 	}} \n 	 \n  \n ";