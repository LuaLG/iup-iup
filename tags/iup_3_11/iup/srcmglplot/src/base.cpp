/***************************************************************************
 * base.cpp is part of Math Graphic Library
 * Copyright (C) 2007-2014 Alexey Balakin <mathgl.abalakin@gmail.ru>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "mgl2/font.h"
#include "mgl2/base.h"
#include "mgl2/eval.h"
//-----------------------------------------------------------------------------
char *mgl_strdup(const char *s)
{
	char *r = (char *)malloc((strlen(s)+1)*sizeof(char));
	memcpy(r,s,(strlen(s)+1)*sizeof(char));
	return r;
}
//-----------------------------------------------------------------------------
void MGL_EXPORT mgl_create_cpp_font(HMGL gr, const wchar_t *how)
{
	unsigned long l=mgl_wcslen(how), i, n=0, m;
	wchar_t ch=*how;
	const mglFont *f = gr->GetFont();
	std::vector<wchar_t> s;	s.push_back(ch);
	for(i=1;i<l;i++)
		if(how[i]==',')	continue;
		else if(how[i]=='-' && i+1<l)
			for(ch++;ch<how[i+1];ch++)	s.push_back(ch);
		else	s.push_back(ch=how[i]);
	for(i=l=n=0;i<s.size();i++)
	{
		ch = f->Internal(s[i]);
		l += 2*f->GetNl(0,ch);
		n += 6*f->GetNt(0,ch);
	}
	printf("unsigned mgl_numg=%lu, mgl_cur=%lu;\n",(unsigned long)s.size(),l+n);
	printf("float mgl_fact=%g;\n",f->GetFact(0)/mgl_fgen);
	printf("long mgl_gen_fnt[%lu][6] = {\n", (unsigned long)s.size());
	for(i=m=0;i<s.size();i++)	// first write symbols descriptions
	{
		ch = f->Internal(s[i]);
		int m1 = f->GetNl(0,ch), m2 = f->GetNt(0,ch);
		printf("\t{0x%x,%d,%d,%lu,%d,%lu},\n",unsigned(s[i]),f->GetWidth(0,ch),m1,m,m2,m+2*m1);
		m += 2*m1+6*m2;
	}
	if(m!=l+n)	printf("#error \"%lu !=%lu + %lu\"",m,l,n);
	printf("};\nshort mgl_buf_fnt[%lu] = {\n",m);
	for(i=0;i<s.size();i++)		// now write data itself
	{
		ch = f->Internal(s[i]);
		unsigned m1 = f->GetNl(0,ch), m2 = f->GetNt(0,ch);
		const short *ln = f->GetLn(0,ch), *tr = f->GetTr(0,ch);
		for(l=0;l<2*m1;l++)	printf("%d,",ln[l]);
		printf("\n");
		for(l=0;l<6*m2;l++)	printf("%d,",tr[l]);
		printf("\n");
	}
	printf("};\n");
}
//-----------------------------------------------------------------------------
void MGL_EXPORT mgl_strtrim(char *str)
{
	if(!str || *str==0)	return;
	size_t n=strlen(str), k, i;
	for(k=0;k<n;k++)	if(str[k]>' ')	break;
	for(i=n;i>k;i--)	if(str[i-1]>' ')	break;
	memmove(str, str+k, (i-k));
	str[i-k]=0;
}
//-----------------------------------------------------------------------------
void MGL_EXPORT mgl_strlwr(char *str)
{
	register size_t k,l=strlen(str);
	for(k=0;k<l;k++)
		str[k] = (str[k]>='A' && str[k]<='Z') ? str[k]+'a'-'A' : str[k];
}
//-----------------------------------------------------------------------------
mglBase::mglBase()
{
	Flag=0;	saved=false;
#if MGL_HAVE_PTHREAD
	pthread_mutex_init(&mutexPnt,0);
	pthread_mutex_init(&mutexTxt,0);
	pthread_mutex_init(&mutexSub,0);
	pthread_mutex_init(&mutexLeg,0);
	pthread_mutex_init(&mutexPrm,0);
	pthread_mutex_init(&mutexPtx,0);
	pthread_mutex_init(&mutexStk,0);
	pthread_mutex_init(&mutexGrp,0);
	pthread_mutex_init(&mutexGlf,0);
	pthread_mutex_init(&mutexAct,0);
	pthread_mutex_init(&mutexDrw,0);
#endif
	fnt=0;	*FontDef=0;	fx=fy=fz=fa=fc=0;
	AMin = mglPoint(0,0,0,0);	AMax = mglPoint(1,1,1,1);

	InUse = 1;	SetQuality();	FaceNum = 0;
	// Always create default palette txt[0] and default scheme txt[1]
#pragma omp critical(txt)
	{
		mglTexture t1(MGL_DEF_PAL,-1), t2(MGL_DEF_SCH,1);
		Txt.reserve(3);
		MGL_PUSH(Txt,t1,mutexTxt);
		MGL_PUSH(Txt,t2,mutexTxt);
	}
	memcpy(last_style,"{k5}-1\0",8);
	MinS=mglPoint(-1,-1,-1);	MaxS=mglPoint(1,1,1);
	fnt = new mglFont;	fnt->gr = this;	PrevState=NAN;
}
mglBase::~mglBase()	{	ClearEq();	delete fnt;	}
//-----------------------------------------------------------------------------
void mglBase::RestoreFont()	{	fnt->Restore();	}
void mglBase::LoadFont(const char *name, const char *path)
{	if(name && *name)	fnt->Load(name,path);	else	fnt->Restore();	}
void mglBase::CopyFont(mglBase *gr)	{	fnt->Copy(gr->GetFont());	}
//-----------------------------------------------------------------------------
mreal mglBase::TextWidth(const char *text, const char *font, mreal size) const
{	return (size<0?-size*FontSize:size)*font_factor*fnt->Width(text,(font&&*font)?font:FontDef)/20.16;	}
mreal mglBase::TextWidth(const wchar_t *text, const char *font, mreal size) const
{	return (size<0?-size*FontSize:size)*font_factor*fnt->Width(text,(font&&*font)?font:FontDef)/20.16;	}
mreal mglBase::TextHeight(const char *font, mreal size) const
{	return (size<0?-size*FontSize:size)*font_factor*fnt->Height(font?font:FontDef)/20.16; }
void mglBase::AddActive(long k,int n)
{
	if(k<0 || (size_t)k>=Pnt.size())	return;
	mglActivePos p;
	const mglPnt &q=Pnt[k];
	int h=GetHeight();
	p.x = int(q.x);	p.y = h>1?h-1-int(q.y):int(q.y);
	p.id = ObjId;	p.n = n;
#pragma omp critical(act)
	MGL_PUSH(Act,p,mutexAct);
}
//-----------------------------------------------------------------------------
mreal mglBase::GetRatio() const	{	return 1;	}
int mglBase::GetWidth() const	{	return 1;	}
int mglBase::GetHeight() const	{	return 1;	}
//-----------------------------------------------------------------------------
void mglBase::StartGroup(const char *name, int id)
{
	LightScale(&B);
	char buf[128];
	snprintf(buf,128,"%s_%d",name,id);
	StartAutoGroup(buf);
}
//-----------------------------------------------------------------------------
const char *mglWarn[mglWarnEnd] = {"data dimension(s) is incompatible",	//mglWarnDim
								"data dimension(s) is too small",		//mglWarnLow
								"minimal data value is negative",		//mglWarnNeg
								"no file or wrong data dimensions",		//mglWarnFile
								"not enough memory", 					//mglWarnMem
								"data values are zero",					//mglWarnZero
								"no legend entries",					//mglWarnLeg
								"slice value is out of range",			//mglWarnSlc
								"number of contours is zero or negative",//mglWarnCnt
								"couldn't open file",					//mglWarnOpen
								"light: ID is out of range",			//mglWarnLId
								"size(s) is zero or negative",			//mglWarnSize
								"format is not supported for that build",//mglWarnFmt
								"axis ranges are incompatible",			//mglWarnTern
								"pointer is NULL",						//mglWarnNull
								"not enough space for plot",			//mglWarnSpc
								"There are wrong argument(s) in script",//mglScrArg
								"There are wrong command in script",	//mglScrCmd
								"There are too long string in script",	//mglScrLong
								"There are unbalanced ' in script"};	//mglScrStr
//-----------------------------------------------------------------------------
void mglBase::SetWarn(int code, const char *who)
{
	WarnCode = code>0 ? code:0;
	if(code>0 && code<mglWarnEnd)
	{
		if(who && *who)	Mess = Mess+"\n"+who+": ";
		else Mess += "\n";
		Mess = Mess+mglWarn[code-1];
	}
	else if(!code)	Mess="";
	else if(who && *who)	Mess = Mess+(code==-2?"":"\n")+who;
	LoadState();
}
//-----------------------------------------------------------------------------
//		Add glyph to the buffer
//-----------------------------------------------------------------------------
void mglGlyph::Create(long Nt, long Nl)
{
	if(Nt<0 || Nl<0)	return;
	nt=Nt;	nl=Nl;
	if(trig)	delete []trig;
	trig = nt>0?new short[6*nt]:0;
	if(line)	delete []line;
	line = nl>0?new short[2*nl]:0;
}
//-----------------------------------------------------------------------------
bool mglGlyph::operator==(const mglGlyph &g)
{
	if(nl!=g.nl || nt!=g.nt)	return false;
	if(trig && memcmp(trig,g.trig,6*nt*sizeof(short)))	return false;
	if(line && memcmp(line,g.line,2*nl*sizeof(short)))	return false;
	return true;
}
//-----------------------------------------------------------------------------
long mglBase::AddGlyph(int s, long j)
{
	// first create glyph for current typeface
	s = s&3;
	mglGlyph g(fnt->GetNt(s,j), fnt->GetNl(s,j));
	memcpy(g.trig, fnt->GetTr(s,j), 6*g.nt*sizeof(short));
	memcpy(g.line, fnt->GetLn(s,j), 2*g.nl*sizeof(short));
	// now let find the similar glyph
	for(size_t i=0;i<Glf.size();i++)	if(g==Glf[i])	return i;
	// if no one then let add it
	long k;
#pragma omp critical(glf)
	{k=Glf.size();	MGL_PUSH(Glf,g,mutexGlf);}	return k;
}
//-----------------------------------------------------------------------------
//		Add points to the buffer
//-----------------------------------------------------------------------------
long mglBase::AddPnt(const mglMatrix *mat, mglPoint p, mreal c, mglPoint n, mreal a, int scl)
{
	// scl=0 -- no scaling
	// scl&1 -- usual scaling
	// scl&2 -- disable NAN at scaling
	// scl&4 -- ???
	// scl&8 -- bypass palette for enabling alpha
	if(mgl_isnan(c) || mgl_isnan(a))	return -1;
	bool norefr = mgl_isnan(n.x) && mgl_isnan(n.y) && !mgl_isnan(n.z);
	if(scl>0)	ScalePoint(mat,p,n,!(scl&2));
	if(mgl_isnan(p.x))	return -1;
	a = (a>=0 && a<=1) ? a : AlphaDef;
	c = (c>=0) ? c:CDef;

	mglPnt q;
	if(get(MGL_REDUCEACC))
	{
		q.x=q.xx=int(p.x*10)*0.1;	q.y=q.yy=int(p.y*10)*0.1;	q.z=q.zz=int(p.z*10)*0.1;
		q.c=int(c*100)*0.01;	q.t=q.ta=int(a*100)*0.01;
		q.u=mgl_isnum(n.x)?int(n.x*100)*0.01:NAN;
		q.v=mgl_isnum(n.y)?int(n.y*100)*0.01:NAN;
		q.w=mgl_isnum(n.z)?int(n.z*100)*0.01:NAN;
	}
	else
	{
		q.x=q.xx=p.x;	q.y=q.yy=p.y;	q.z=q.zz=p.z;
		q.c=c;	q.t=q.ta=a;	q.u=n.x;	q.v=n.y;	q.w=n.z;
	}
	register long ci=long(c);
	if(ci<0 || ci>=(long)Txt.size())	ci=0;	// NOTE never should be here!!!
	const mglTexture &txt=Txt[ci];
	txt.GetC(c,a,q);	// RGBA color

	// add gap for texture coordinates for compatibility with OpenGL
	const mreal gap = 0./MGL_TEXTURE_COLOURS;
	q.c = ci+(q.c-ci)*(1-2*gap)+gap;
	q.t = q.t*(1-2*gap)+gap;
	q.ta = q.t;

	if(scl&8 && scl>0)	q.a=a;	// bypass palette for enabling alpha in Error()
	if(!get(MGL_ENABLE_ALPHA))	{	q.a=1;	if(txt.Smooth!=2)	q.ta=1-gap;	}
	if(norefr)	q.v=0;
	if(!get(MGL_ENABLE_LIGHT) && !(scl&4))	q.u=q.v=NAN;
	if(mat->norot)	q.sub=-1;	// NOTE: temporary -- later should be mglInPlot here
	long k;
#pragma omp critical(pnt)
	{k=Pnt.size();	MGL_PUSH(Pnt,q,mutexPnt);}	return k;
}
//-----------------------------------------------------------------------------
long mglBase::CopyNtoC(long from, mreal c)
{
	if(from<0)	return -1;
	mglPnt p=Pnt[from];
	if(mgl_isnum(c))	{	p.c=c;	p.t=0;	Txt[long(c)].GetC(c,0,p);	}
	long k;
#pragma omp critical(pnt)
	{k=Pnt.size();	MGL_PUSH(Pnt,p,mutexPnt);}	return k;
}
//-----------------------------------------------------------------------------
long mglBase::CopyProj(long from, mglPoint p, mglPoint n)
{
	if(from<0)	return -1;
	mglPnt q=Pnt[from];
	q.x=q.xx=p.x;	q.y=q.yy=p.y;	q.z=q.zz=p.z;
	q.u = n.x;		q.v = n.y;		q.w = n.z;
	long k;
#pragma omp critical(pnt)
	{k=Pnt.size();	MGL_PUSH(Pnt,q,mutexPnt);}	return k;
}
//-----------------------------------------------------------------------------
void mglBase::Reserve(long n)
{
	if(TernAxis&4)	n*=4;
#pragma omp critical(pnt)
	Pnt.reserve(n);
#pragma omp critical(prm)
	Prm.reserve(n);
}
//-----------------------------------------------------------------------------
//		Boundaries and scaling
//-----------------------------------------------------------------------------
bool mglBase::RecalcCRange()
{
	bool wrong=false;
	if(!fa)
	{	FMin.c = Min.c;	FMax.c = Max.c;	}
	else
	{
		FMin.c = INFINITY;	FMax.c = -INFINITY;
		register int i;
		mreal a;
		int n=30;
		for(i=0;i<=n;i++)
		{
			a = fa->Calc(0,0,0,Min.c+i*(Max.c-Min.c)/n);
			if(mgl_isbad(a))	wrong=true;
			if(a<FMin.c)	FMin.c=a;
			if(a>FMax.c)	FMax.c=a;
		}
	}
	return wrong;
}
//-----------------------------------------------------------------------------
void mglBase::RecalcBorder()
{
	ZMin = 1.;
	bool wrong=false;
	if(!fx && !fy && !fz)
	{	FMin = Min;	FMax = Max;	}
	else
	{
		FMin = mglPoint( INFINITY, INFINITY, INFINITY);
		FMax = mglPoint(-INFINITY,-INFINITY,-INFINITY);
		register int i,j;
		int n=30;
		for(i=0;i<=n;i++)	for(j=0;j<=n;j++)	// x range
		{
			if(SetFBord(Min.x, Min.y+i*(Max.y-Min.y)/n, Min.z+j*(Max.z-Min.z)/n))	wrong=true;
			if(SetFBord(Max.x, Min.y+i*(Max.y-Min.y)/n, Min.z+j*(Max.z-Min.z)/n))	wrong=true;
		}
		for(i=0;i<=n;i++)	for(j=0;j<=n;j++)	// y range
		{
			if(SetFBord(Min.x+i*(Max.x-Min.x)/n, Min.y, Min.z+j*(Max.z-Min.z)/n))	wrong=true;
			if(SetFBord(Min.x+i*(Max.x-Min.x)/n, Max.y, Min.z+j*(Max.z-Min.z)/n))	wrong=true;
		}
		for(i=0;i<=n;i++)	for(j=0;j<=n;j++)	// x range
		{
			if(SetFBord(Min.x+i*(Max.x-Min.x)/n, Min.y+j*(Max.y-Min.y)/n, Min.z))	wrong=true;
			if(SetFBord(Min.x+i*(Max.x-Min.x)/n, Min.y+j*(Max.y-Min.y)/n, Max.z))	wrong=true;
		}
		mreal d;
		if(!fx)	{	FMin.x = Min.x;	FMax.x = Max.x;	}
		else	{	d=0.01*(FMax.x-FMin.x);	FMin.x-=d;	FMax.x+=d;	}
		if(!fy)	{	FMin.y = Min.y;	FMax.y = Max.y;	}
		else	{	d=0.01*(FMax.y-FMin.y);	FMin.y-=d;	FMax.y+=d;	}
		if(!fz)	{	FMin.z = Min.z;	FMax.z = Max.z;	}
		else	{	d=0.01*(FMax.z-FMin.z);	FMin.z-=d;	FMax.z+=d;	}
	}
	if(RecalcCRange())	wrong=true;
	if(wrong)	SetWarn(mglWarnTern, "Curved coordinates");
}
//-----------------------------------------------------------------------------
bool mglBase::SetFBord(mreal x,mreal y,mreal z)
{
	bool wrong=false;
	if(fx)
	{
		mreal v = fx->Calc(x,y,z);
		if(mgl_isbad(v))	wrong = true;
		if(FMax.x < v)	FMax.x = v;
		if(FMin.x > v)	FMin.x = v;
	}
	if(fy)
	{
		mreal v = fy->Calc(x,y,z);
		if(mgl_isbad(v))	wrong = true;
		if(FMax.y < v)	FMax.y = v;
		if(FMin.y > v)	FMin.y = v;
	}
	if(fz)
	{
		mreal v = fz->Calc(x,y,z);
		if(mgl_isbad(v))	wrong = true;
		if(FMax.z < v)	FMax.z = v;
		if(FMin.z > v)	FMin.z = v;
	}
	return wrong;
}
//-----------------------------------------------------------------------------
bool mglBase::ScalePoint(const mglMatrix *, mglPoint &p, mglPoint &n, bool use_nan) const
{
	mreal &x=p.x, &y=p.y, &z=p.z;
	if(mgl_isnan(x) || mgl_isnan(y) || mgl_isnan(z))	{	x=NAN;	return false;	}
	mreal x1,y1,z1,x2,y2,z2;
	x1 = x>0?x*MGL_EPSILON:x/MGL_EPSILON;	x2 = x<0?x*MGL_EPSILON:x/MGL_EPSILON;
	y1 = y>0?y*MGL_EPSILON:y/MGL_EPSILON;	y2 = y<0?y*MGL_EPSILON:y/MGL_EPSILON;
	z1 = z>0?z*MGL_EPSILON:z/MGL_EPSILON;	z2 = z<0?z*MGL_EPSILON:z/MGL_EPSILON;
	bool res = true;
	if(x2>CutMin.x && x1<CutMax.x && y2>CutMin.y && y1<CutMax.y &&
		z2>CutMin.z && z1<CutMax.z)	res = false;
	if(fc && fc->Calc(x,y,z)!=0)	res = false;

	if(get(MGL_ENABLE_CUT) || !use_nan)
	{
//		if(x1<Min.x || x2>Max.x || y1<Min.y || y2>Max.y || z1<Min.z || z2>Max.z)	res = false;
		if((x1-Min.x)*(x1-Max.x)>0 && (x2-Min.x)*(x2-Max.x)>0)	res = false;
		if((y1-Min.y)*(y1-Max.y)>0 && (y2-Min.y)*(y2-Max.y)>0)	res = false;
		if((z1-Min.z)*(z1-Max.z)>0 && (z2-Min.z)*(z2-Max.z)>0)	res = false;
	}
	else
	{
		if(x1<Min.x)	{x=Min.x;	n=mglPoint(1,0,0);}
		if(x2>Max.x)	{x=Max.x;	n=mglPoint(1,0,0);}
		if(y1<Min.y)	{y=Min.y;	n=mglPoint(0,1,0);}
		if(y2>Max.y)	{y=Max.y;	n=mglPoint(0,1,0);}
		if(z1<Min.z)	{z=Min.z;	n=mglPoint(0,0,1);}
		if(z2>Max.z)	{z=Max.z;	n=mglPoint(0,0,1);}
	}

	x1=x;	y1=y;	z1=z;	x2=y2=z2=1;
	if(fx)	{	x1 = fx->Calc(x,y,z);	x2 = fx->CalcD('x',x,y,z);	}
	if(fy)	{	y1 = fy->Calc(x,y,z);	y2 = fy->CalcD('y',x,y,z);	}
	if(fz)	{	z1 = fz->Calc(x,y,z);	z2 = fz->CalcD('z',x,y,z);	}
	if(mgl_isnan(x1) || mgl_isnan(y1) || mgl_isnan(z1))	{	x=NAN;	return false;	}

	register mreal d;	// TODO: should I update normale for infinite light source (x=NAN)?!?
	d = 1/(FMax.x - FMin.x);	x = (2*x1 - FMin.x - FMax.x)*d;	x2 *= 2*d;
	d = 1/(FMax.y - FMin.y);	y = (2*y1 - FMin.y - FMax.y)*d;	y2 *= 2*d;
	d = 1/(FMax.z - FMin.z);	z = (2*z1 - FMin.z - FMax.z)*d;	z2 *= 2*d;
	n.x *= y2*z2;	n.y *= x2*z2;	n.z *= x2*y2;
	if((TernAxis&3)==1)	// usual ternary axis
	{
		if(x+y>0)
		{
			if(get(MGL_ENABLE_CUT))	res = false;
			else	y = -x;
		}
		x += (y+1)/2;	n.x += n.y/2;
	}
	else if((TernAxis&3)==2)	// quaternary axis
	{
		if(x+y+z>-1)
		{
			if(get(MGL_ENABLE_CUT))	res = false;
			else	z = -1-y-x;
		}
		x += 1+(y+z)/2;		y += (z+1)/3;
		n.x += (n.y+n.z)/2;	n.y += n.z/3;
	}
	if(fabs(x)>MGL_FEPSILON || fabs(y)>MGL_FEPSILON || fabs(z)>MGL_FEPSILON)	res = false;

	if(!res && use_nan)	x = NAN;	// extra sign that point shouldn't be plotted
	return res;
}
//-----------------------------------------------------------------------------
//		Ranges
//-----------------------------------------------------------------------------
void mglScaleAxis(mreal &v1, mreal &v2, mreal &v0, mreal x1, mreal x2)
{
	if(x1==x2 || v1==v2)	return;
	mreal dv,d0;	x2-=1;
	if(v1*v2>0 && (v2/v1>=100 || v2/v1<=0.01))	// log scale
	{
		dv=log(v2/v1);	d0 = log(v0/v1)/log(v2/v1);
		v1*=exp(dv*x1);	v2*=exp(dv*x2);	v0=v1*exp(d0*log(v2/v1));
	}
	else
	{
		dv=v2-v1;	d0=(v0-v1)/(v2-v1);
		v1+=dv*x1;	v2+=dv*x2;	v0=v1+d0*(v2-v1);
	}
}
//-----------------------------------------------------------------------------
void mglBase::SetOrigin(mreal x0, mreal y0, mreal z0, mreal c0)
{
	Org=mglPoint(x0,y0,z0,c0);
	if((TernAxis&3)==0)
	{
		Min = OMin;	Max = OMax;
		mglScaleAxis(Min.x, Max.x, Org.x, AMin.x, AMax.x);
		mglScaleAxis(Min.y, Max.y, Org.y, AMin.y, AMax.y);
		mglScaleAxis(Min.z, Max.z, Org.z, AMin.z, AMax.z);
		mglScaleAxis(Min.c, Max.c, Org.c, AMin.c, AMax.c);
	}
}
//-----------------------------------------------------------------------------
void mglBase::SetRanges(mglPoint m1, mglPoint m2)
{
	if(m1.x!=m2.x)	{	Min.x=m1.x;	Max.x=m2.x;	}
	if(m1.y!=m2.y)	{	Min.y=m1.y;	Max.y=m2.y;	}
	if(m1.z!=m2.z)	{	Min.z=m1.z;	Max.z=m2.z;	}
	if(m1.c!=m2.c)	{	Min.c=m1.c;	Max.c=m2.c;	}
	else			{	Min.c=Min.z;Max.c=Max.z;}

	if(Org.x<Min.x && mgl_isnum(Org.x))	Org.x = Min.x;
	if(Org.x>Max.x && mgl_isnum(Org.x))	Org.x = Max.x;
	if(Org.y<Min.y && mgl_isnum(Org.y))	Org.y = Min.y;
	if(Org.y>Max.y && mgl_isnum(Org.y))	Org.y = Max.y;
	if(Org.z<Min.z && mgl_isnum(Org.z))	Org.z = Min.z;
	if(Org.z>Max.z && mgl_isnum(Org.z))	Org.z = Max.z;

	if((TernAxis&3)==0)
	{
		OMax = Max;	OMin = Min;
		mglScaleAxis(Min.x, Max.x, Org.x, AMin.x, AMax.x);
		mglScaleAxis(Min.y, Max.y, Org.y, AMin.y, AMax.y);
		mglScaleAxis(Min.z, Max.z, Org.z, AMin.z, AMax.z);
		mglScaleAxis(Min.c, Max.c, Org.c, AMin.c, AMax.c);
	}

	CutMin = mglPoint(0,0,0);	CutMax = mglPoint(0,0,0);
	RecalcBorder();
}
//-----------------------------------------------------------------------------
void mglBase::CRange(HCDT a,bool add, mreal fact)
{
	mreal v1=a->Minimal(), v2=a->Maximal(), dv;
	dv=(v2-v1)*fact;	v1 -= dv;	v2 += dv;
	CRange(v1,v2,add);
}
void mglBase::CRange(mreal v1,mreal v2,bool add)
{
	if(v1==v2 && !add)	return;
	if(!add)
	{
		if(v1==v1)	Min.c = v1;	
		if(v2==v2)	Max.c = v2;		
	}
	else if(Min.c<Max.c)
	{
		if(Min.c>v1)	Min.c=v1;
		if(Max.c<v2)	Max.c=v2;
	}
	else
	{
		mreal dv = Min.c;
		Min.c = v1<Max.c ? v1:Max.c;
		Max.c = v2>dv ? v2:dv;
	}
	if(Org.c<Min.c && mgl_isnum(Org.c))	Org.c = Min.c;
	if(Org.c>Max.c && mgl_isnum(Org.c))	Org.c = Max.c;
	if((TernAxis&3)==0)
	{
		OMax.c = Max.c;	OMin.c = Min.c;
		mglScaleAxis(Min.c, Max.c, Org.c, AMin.c, AMax.c);
	}
	RecalcCRange();
}
//-----------------------------------------------------------------------------
void mglBase::XRange(HCDT a,bool add,mreal fact)
{
	mreal v1=a->Minimal(), v2=a->Maximal(), dv;
	dv=(v2-v1)*fact;	v1 -= dv;	v2 += dv;
	XRange(v1,v2,add);
}
void mglBase::XRange(mreal v1,mreal v2,bool add)
{
	if(v1==v2 && !add)	return;
	if(!add)
	{
		if(v1==v1)	Min.x = v1;	
		if(v2==v2)	Max.x = v2;		
	}
	else if(Min.x<Max.x)
	{
		if(Min.x>v1)	Min.x=v1;
		if(Max.x<v2)	Max.x=v2;
	}
	else
	{
		mreal dv = Min.x;
		Min.x = v1<Max.x ? v1:Max.x;
		Max.x = v2>dv ? v2:dv;
	}
	if(Org.x<Min.x && mgl_isnum(Org.x))	Org.x = Min.x;
	if(Org.x>Max.x && mgl_isnum(Org.x))	Org.x = Max.x;
	if((TernAxis&3)==0)
	{
		OMax.x = Max.x;	OMin.x = Min.x;
		mglScaleAxis(Min.x, Max.x, Org.x, AMin.x, AMax.x);
	}
	RecalcBorder();
}
//-----------------------------------------------------------------------------
void mglBase::YRange(HCDT a,bool add,mreal fact)
{
	mreal v1=a->Minimal(), v2=a->Maximal(), dv;
	dv=(v2-v1)*fact;	v1 -= dv;	v2 += dv;
	YRange(v1,v2,add);
}
void mglBase::YRange(mreal v1,mreal v2,bool add)
{
	if(v1==v2 && !add)	return;
	if(!add)
	{
		if(v1==v1)	Min.y = v1;	
		if(v2==v2)	Max.y = v2;		
	}
	else if(Min.y<Max.y)
	{
		if(Min.y>v1)	Min.y=v1;
		if(Max.y<v2)	Max.y=v2;
	}
	else
	{
		mreal dv = Min.y;
		Min.y = v1<Max.y ? v1:Max.y;
		Max.y = v2>dv ? v2:dv;
	}
	if(Org.y<Min.y && mgl_isnum(Org.y))	Org.y = Min.y;
	if(Org.y>Max.y && mgl_isnum(Org.y))	Org.y = Max.y;
	if((TernAxis&3)==0)
	{
		OMax.y = Max.y;	OMin.y = Min.y;
		mglScaleAxis(Min.y, Max.y, Org.y, AMin.y, AMax.y);

	}
	RecalcBorder();
}
//-----------------------------------------------------------------------------
void mglBase::ZRange(HCDT a,bool add,mreal fact)
{
	mreal v1=a->Minimal(), v2=a->Maximal(), dv;
	dv=(v2-v1)*fact;	v1 -= dv;	v2 += dv;
	ZRange(v1,v2,add);
}
void mglBase::ZRange(mreal v1,mreal v2,bool add)
{
	if(v1==v2 && !add)	return;
	if(!add)
	{
		if(v1==v1)	Min.z = v1;	
		if(v2==v2)	Max.z = v2;		
	}
	else if(Min.z<Max.z)
	{
		if(Min.z>v1)	Min.z=v1;
		if(Max.z<v2)	Max.z=v2;
	}
	else
	{
		mreal dv = Min.z;
		Min.z = v1<Max.z ? v1:Max.z;
		Max.z = v2>dv ? v2:dv;
	}
	if(Org.z<Min.z && mgl_isnum(Org.z))	Org.z = Min.z;
	if(Org.z>Max.z && mgl_isnum(Org.z))	Org.z = Max.z;
	if((TernAxis&3)==0)
	{
		OMax.z = Max.z;	OMin.z = Min.z;
		mglScaleAxis(Min.z, Max.z, Org.z, AMin.z, AMax.z);
	}
	RecalcBorder();
}
//-----------------------------------------------------------------------------
void mglBase::SetAutoRanges(mreal x1, mreal x2, mreal y1, mreal y2, mreal z1, mreal z2, mreal c1, mreal c2)
{
	if(x1!=x2)	{	Min.x = x1;	Max.x = x2;	}
	if(y1!=y2)	{	Min.y = y1;	Max.y = y2;	}
	if(z1!=z2)	{	Min.z = z1;	Max.z = z2;	}
	if(c1!=c2)	{	Min.c = c1;	Max.c = c2;	}
}
//-----------------------------------------------------------------------------
void mglBase::Ternary(int t)
{
	static mglPoint x1(-1,-1,-1),x2(1,1,1),o(NAN,NAN,NAN);
	static bool c = true;
	TernAxis = t;
	if(t&3)
	{
		if(c)	{	x1 = Min;	x2 = Max;	o = Org;	}
		SetRanges(mglPoint(0,0,0),mglPoint(1,1,(t&3)==1?0:1));
		Org=mglPoint(0,0,(t&3)==1?NAN:0);	c = false;
	}
	else if(!c)	{	SetRanges(x1,x2);	Org=o;	c=true;	}
}
//-----------------------------------------------------------------------------
//		Transformation functions
//-----------------------------------------------------------------------------
void mglBase::SetFunc(const char *EqX,const char *EqY,const char *EqZ,const char *EqA)
{
	if(fa)	delete fa;	if(fx)	delete fx;
	if(fy)	delete fy;	if(fz)	delete fz;
	if(EqX && *EqX && (EqX[0]!='x' || EqX[1]!=0))
		fx = new mglFormula(EqX);
	else	fx = 0;
	if(EqY && *EqY && (EqY[0]!='y' || EqY[1]!=0))
		fy = new mglFormula(EqY);
	else	fy = 0;
	if(EqZ && *EqZ && (EqZ[0]!='z' || EqZ[1]!=0))
		fz = new mglFormula(EqZ);
	else	fz = 0;
	if(EqA && *EqA && ((EqA[0]!='c' && EqA[0]!='a') || EqA[1]!=0))
		fa = new mglFormula(EqA);
	else	fa = 0;
	RecalcBorder();
}
//-----------------------------------------------------------------------------
void mglBase::CutOff(const char *EqC)
{
	if(fc)	delete fc;
	if(EqC && EqC[0])	fc = new mglFormula(EqC);	else	fc = 0;
}
//-----------------------------------------------------------------------------
void mglBase::SetCoor(int how)
{
	switch(how)
	{
	case mglCartesian:	SetFunc(0,0);	break;
	case mglPolar:
		SetFunc("x*cos(y)","x*sin(y)");	break;
	case mglSpherical:
		SetFunc("x*sin(y)*cos(z)","x*sin(y)*sin(z)","x*cos(y)");	break;
	case mglParabolic:
		SetFunc("x*y","(x*x-y*y)/2");	break;
	case mglParaboloidal:
		SetFunc("(x*x-y*y)*cos(z)/2","(x*x-y*y)*sin(z)/2","x*y");	break;
	case mglOblate:
		SetFunc("cosh(x)*cos(y)*cos(z)","cosh(x)*cos(y)*sin(z)","sinh(x)*sin(y)");	break;
//		SetFunc("x*y*cos(z)","x*y*sin(z)","(x*x-1)*(1-y*y)");	break;
	case mglProlate:
		SetFunc("sinh(x)*sin(y)*cos(z)","sinh(x)*sin(y)*sin(z)","cosh(x)*cos(y)");	break;
	case mglElliptic:
		SetFunc("cosh(x)*cos(y)","sinh(x)*sin(y)");	break;
	case mglToroidal:
		SetFunc("sinh(x)*cos(z)/(cosh(x)-cos(y))","sinh(x)*sin(z)/(cosh(x)-cos(y))",
			"sin(y)/(cosh(x)-cos(y))");	break;
	case mglBispherical:
		SetFunc("sin(y)*cos(z)/(cosh(x)-cos(y))","sin(y)*sin(z)/(cosh(x)-cos(y))",
			"sinh(x)/(cosh(x)-cos(y))");	break;
	case mglBipolar:
		SetFunc("sinh(x)/(cosh(x)-cos(y))","sin(y)/(cosh(x)-cos(y))");	break;
	case mglLogLog:	SetFunc("lg(x)","lg(y)");	break;
	case mglLogX:	SetFunc("lg(x)","");	break;
	case mglLogY:	SetFunc("","lg(y)");	break;
	default:	SetFunc(0,0);	break;
	}
}
//-----------------------------------------------------------------------------
void mglBase::ClearEq()
{
	if(fx)	delete fx;	if(fy)	delete fy;	if(fz)	delete fz;
	if(fa)	delete fa;	if(fc)	delete fc;
	fx = fy = fz = fc = fa = 0;
	RecalcBorder();
}
//-----------------------------------------------------------------------------
//		Colors ids
//-----------------------------------------------------------------------------
MGL_EXPORT mglColorID mglColorIds[31] = {{'k', mglColor(0,0,0)},
	{'r', mglColor(1,0,0)},		{'R', mglColor(0.5,0,0)},
	{'g', mglColor(0,1,0)},		{'G', mglColor(0,0.5,0)},
	{'b', mglColor(0,0,1)},		{'B', mglColor(0,0,0.5)},
	{'w', mglColor(1,1,1)},		{'W', mglColor(0.7,0.7,0.7)},
	{'c', mglColor(0,1,1)},		{'C', mglColor(0,0.5,0.5)},
	{'m', mglColor(1,0,1)},		{'M', mglColor(0.5,0,0.5)},
	{'y', mglColor(1,1,0)},		{'Y', mglColor(0.5,0.5,0)},
	{'h', mglColor(0.5,0.5,0.5)},	{'H', mglColor(0.3,0.3,0.3)},
	{'l', mglColor(0,1,0.5)},	{'L', mglColor(0,0.5,0.25)},
	{'e', mglColor(0.5,1,0)},	{'E', mglColor(0.25,0.5,0)},
	{'n', mglColor(0,0.5,1)},	{'N', mglColor(0,0.25,0.5)},
	{'u', mglColor(0.5,0,1)},	{'U', mglColor(0.25,0,0.5)},
	{'q', mglColor(1,0.5,0)},	{'Q', mglColor(0.5,0.25,0)},
	{'p', mglColor(1,0,0.5)},	{'P', mglColor(0.5,0,0.25)},
	{' ', mglColor(-1,-1,-1)},	{0, mglColor(-1,-1,-1)}	// the last one MUST have id=0
};
//-----------------------------------------------------------------------------
void MGL_EXPORT mgl_chrrgb(char p, float c[3])
{
	c[0]=c[1]=c[2]=-1;
	for(long i=0; mglColorIds[i].id; i++)
		if(mglColorIds[i].id==p)
		{
			c[0]=mglColorIds[i].col.r;
			c[1]=mglColorIds[i].col.g;
			c[2]=mglColorIds[i].col.b;
			break;
		}
}
//-----------------------------------------------------------------------------
void mglTexture::Set(const char *s, int smooth, mreal alpha)
{
	// NOTE: New syntax -- colors are CCCCC or {CNCNCCCN}; options inside []
	if(!s || !s[0])	return;
	strncpy(Sch,s,259);	Smooth=smooth;	Alpha=alpha;

	register long i,j=0,m=0,l=strlen(s);
	const char *dig = "0123456789abcdefABCDEF";
	bool map = smooth==2 || mglchr(s,'%'), sm = smooth>=0 && !strchr(s,'|');	// Use mapping, smoothed colors
	for(i=0;i<l;i++)		// find number of colors
	{
		if(smooth>=0 && s[i]==':' && j<1)	break;
		if(s[i]=='[')	j++;	if(s[i]==']')	j--;
		if(strchr(MGL_COLORS,s[i]) && j<1)	n++;
		if(s[i]=='x' && i>0 && s[i-1]=='{' && j<1)	n++;
//		if(smooth && s[i]==':')	break;	// NOTE: should use []
	}
	if(!n)
	{
		if((strchr(s,'|') || strchr(s,'!')) && !smooth)	// sharp colors
		{	n=l=6;	s=MGL_DEF_SCH;	sm = false;	}
		else if(smooth==0)		// none colors but color scheme
		{	n=l=6;	s=MGL_DEF_SCH;	}
		else	return;
	}
	bool man=sm;
	mglColor *c = new mglColor[2*n];		// Colors itself
	mreal *val = new mreal[n];
	for(i=j=n=0;i<l;i++)	// fill colors
	{
		if(smooth>=0 && s[i]==':' && j<1)	break;
		if(s[i]=='[')	j++;	if(s[i]==']')	j--;
		if(s[i]=='{')	m++;	if(s[i]=='}')	m--;
		if(strchr(MGL_COLORS,s[i]) && j<1 && (m==0 || s[i-1]=='{'))	// {CN,val} format, where val in [0,1]
		{
			if(m>0 && s[i+1]>'0' && s[i+1]<='9')// ext color
			{	c[2*n] = mglColor(s[i],(s[i+1]-'0')/5.f);	i++;	}
			else	c[2*n] = mglColor(s[i]);	// usual color
			val[n]=-1;	n++;
		}
		if(s[i]=='x' && i>0 && s[i-1]=='{' && j<1)	// {xRRGGBB,val} format, where val in [0,1]
		{
			if(strchr(dig,s[i+1]) && strchr(dig,s[i+2]) && strchr(dig,s[i+3]) && strchr(dig,s[i+4]) && strchr(dig,s[i+5]) && strchr(dig,s[i+6]))
			{
				char ss[3]="00";	c[2*n].a = 1;
				ss[0] = s[i+1];	ss[1] = s[i+2];	c[2*n].r = strtol(ss,0,16)/255.;
				ss[0] = s[i+3];	ss[1] = s[i+4];	c[2*n].g = strtol(ss,0,16)/255.;
				ss[0] = s[i+5];	ss[1] = s[i+6];	c[2*n].b = strtol(ss,0,16)/255.;
				if(strchr(dig,s[i+7]) && strchr(dig,s[i+8]))
				{	ss[0] = s[i+7];	ss[1] = s[i+8];	c[2*n].a = strtol(ss,0,16)/255.;	i+=2;	}
				val[n]=-1;	i+=6;	n++;
			}
		}
		if(s[i]==',' && m>0 && j<1 && n>0)
			val[n-1] = atof(s+i+1);
		// NOTE: User can change alpha if it placed like {AN}
		if(s[i]=='A' && j<1 && m>0 && s[i+1]>'0' && s[i+1]<='9')
		{	man=false;	alpha = 0.1*(s[i+1]-'0');	i++;	}
	}
	for(long i=0;i<n;i++)	// default texture
	{	c[2*i+1]=c[2*i];	c[2*i].a=man?0:alpha;	c[2*i+1].a=alpha;	}
	if(map && sm)		// map texture
	{
		if(n==2)
		{	c[1]=c[2];	c[2]=c[0];	c[0]=BC;	c[3]=c[1]+c[2];	}
		else if(n==3)
		{	c[1]=c[2];	c[2]=c[0];	c[0]=BC;	c[3]=c[4];	n=2;}
		else
		{	c[1]=c[4];	c[3]=c[6];	n=2;	}
		c[0].a = c[1].a = c[2].a = c[3].a = alpha;
		val[0]=val[1]=-1;
	}
	// TODO if(!sm && n==1)	then try to find color in palette ???

	// fill missed values  of val[]
	float  v1=0,v2=1;
	std::vector <long>  def;
	val[0]=0;	val[n-1]=1;	// boundary have to be [0,1]
	for(long i=0;i<n;i++) if(val[i]>0 && val[i]<1) 	def.push_back(i);
	def.push_back(n-1);
	long i1=0,i2;
	for(size_t j=0;j<def.size();j++)	for(i=i1+1;i<def[j];i++)
	{
		i1 = j>0?def[j-1]:0;	i2 = def[j];
		v1 = val[i1];	v2 = val[i2];
		v2 = i2-i1>1?(v2-v1)/(i2-i1):0;
		val[i]=v1+v2*(i-i1);
	}
	// fill texture itself
	mreal v=sm?(n-1)/255.:n/256.;
	if(!sm)
//#pragma omp parallel for	// remove parallel here due to possible race conditions for v<1
		for(long i=0;i<256;i++)
		{
			register long j = 2*long(v*i);	//u-=j;
			col[2*i] = c[j];	col[2*i+1] = c[j+1];
		}
	else	for(i=i1=0;i<256;i++)
	{
		register mreal u = v*i;	j = long(u);	//u-=j;
		if(j<n-1)	// advanced scheme using val
		{
			for(;i1<n-1 && i>=255*val[i1];i1++);
			v2 = i1<n?1/(val[i1]-val[i1-1]):0;
			j=i1-1;	u=(i/255.-val[j])*v2;
			col[2*i] = c[2*j]*(1-u)+c[2*j+2]*u;
			col[2*i+1]=c[2*j+1]*(1-u)+c[2*j+3]*u;
		}
		else
		{	col[2*i] = c[2*n-2];col[2*i+1] = c[2*n-1];	}
	}
	delete []c;	delete []val;
}
//-----------------------------------------------------------------------------
mglColor mglTexture::GetC(mreal u,mreal v) const
{
	u -= long(u);
	register long i=long(255*u);	u = u*255-i;
	const mglColor *s=col+2*i;	//mglColor p;
	return (s[0]*(1-u)+s[2]*u)*(1-v) + (s[1]*(1-u)+s[3]*u)*v;
// 	p.r = (s[0].r*(1-u)+s[2].r*u)*(1-v) + (s[1].r*(1-u)+s[3].r*u)*v;
// 	p.g = (s[0].g*(1-u)+s[2].g*u)*(1-v) + (s[1].g*(1-u)+s[3].g*u)*v;
// 	p.b = (s[0].b*(1-u)+s[2].b*u)*(1-v) + (s[1].b*(1-u)+s[3].b*u)*v;
// 	p.a = (s[0].a*(1-u)+s[2].a*u)*(1-v) + (s[1].a*(1-u)+s[3].a*u)*v;
// 	return p;
}
//-----------------------------------------------------------------------------
void mglTexture::GetC(mreal u,mreal v,mglPnt &p) const
{
	u -= long(u);
	register long i=long(255*u);	u = u*255-i;
	const mglColor *s=col+2*i;
	p.r = (s[0].r*(1-u)+s[2].r*u)*(1-v) + (s[1].r*(1-u)+s[3].r*u)*v;
	p.g = (s[0].g*(1-u)+s[2].g*u)*(1-v) + (s[1].g*(1-u)+s[3].g*u)*v;
	p.b = (s[0].b*(1-u)+s[2].b*u)*(1-v) + (s[1].b*(1-u)+s[3].b*u)*v;
	p.a = (s[0].a*(1-u)+s[2].a*u)*(1-v) + (s[1].a*(1-u)+s[3].a*u)*v;
//	p.a = (s[0].a*(1-u)+s[2].a*u)*v + (s[1].a*(1-u)+s[3].a*u)*(1-v);	// for alpha use inverted
}
//-----------------------------------------------------------------------------
long mglBase::AddTexture(const char *cols, int smooth)
{
	if(smooth>=0)	SetMask(cols);
	mglTexture t(cols,smooth,smooth==2?AlphaDef:1);
	if(t.n==0)	return smooth<0 ? 0:1;
	if(smooth<0)	CurrPal=0;
	// check if already exist
	for(size_t i=0;i<Txt.size();i++)	if(t.IsSame(Txt[i]))	return i;
	// create new one
	long k;
#pragma omp critical(txt)
	{k=Txt.size();	MGL_PUSH(Txt,t,mutexTxt);}	return k;
}
//-----------------------------------------------------------------------------
mreal mglBase::AddTexture(mglColor c)
{
	if(!c.Valid())	return -1;
	// first lets try an existed one
	for(size_t i=0;i<Txt.size();i++)	for(int j=0;j<255;j++)
		if(c==Txt[i].col[2*j])
			return i+j/255.;
	// add new texture
	mglTexture t;
#pragma omp parallel for
	for(long i=0;i<MGL_TEXTURE_COLOURS;i++)	t.col[i]=c;
	long k;
#pragma omp critical(txt)
	{k=Txt.size();	MGL_PUSH(Txt,t,mutexTxt);}	return k;
}
//-----------------------------------------------------------------------------
//		Coloring and palette
//-----------------------------------------------------------------------------
mreal mglBase::NextColor(long &id)
{
	long i=abs(id)/256, n=Txt[i].n, p=abs(id)&0xff;
	if(id>=0)	{	p=(p+1)%n;	id = 256*i+p;	}
	mglColor c = Txt[i].col[int(MGL_TEXTURE_COLOURS*(p+0.5)/n)];
	mreal dif, dmin=1;
	// try to find closest color
	for(long j=0;mglColorIds[j].id;j++)	for(long k=1;k<10;k++)
	{
		mglColor cc;	cc.Set(mglColorIds[j].col,k/5.);
		dif = (c-cc).NormS();
		if(dif<dmin)
		{
			last_style[1] = mglColorIds[j].id;
			last_style[2] = k+'0';
			dmin=dif;
		}
	}
	if(!leg_str.empty())
	{	AddLegend(leg_str.c_str(),last_style);	leg_str.clear();	}
	CDef = i + (n>0 ? (p+0.5)/n : 0);	CurrPal++;
	return CDef;
}
//-----------------------------------------------------------------------------
mreal mglBase::NextColor(long id, long sh)
{
	long i=abs(id)/256, n=Txt[i].n, p=abs(id)&0xff;
	if(id>=0)	p=(p+sh)%n;
	return i + (n>0 ? (p+0.5)/n : 0);
}
//-----------------------------------------------------------------------------
const char *mglchrs(const char *str, const char *chr)
{
	if(!str || !str[0] || !chr || !chr[0])	return NULL;
	size_t l=strlen(chr);
	for(size_t i=0;i<l;i++)
	{
		const char *res = mglchr(str,chr[i]);
		if(res)	return res;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
const char *mglchr(const char *str, char ch)
{
	if(!str || !str[0])	return NULL;
	size_t l=strlen(str),k=0;
	for(size_t i=0;i<l;i++)
	{
		register char c = str[i];
		if(c=='{')	k++;
		if(c=='}')	k--;
		if(c==ch && k==0)	return str+i;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
char mglBase::SetPenPal(const char *p, long *Id, bool pal)
{
	char mk=0;
	PDef = 0xffff;	// reset to solid line
	memcpy(last_style,"{k5}-1\0",8);

	Arrow1 = Arrow2 = 0;	PenWidth = 1;
	if(p && *p)
	{
//		const char *col = "wkrgbcymhRGBCYMHWlenuqpLENUQP";
		unsigned val[8] = {0x0000, 0xffff, 0x00ff, 0x0f0f, 0x1111, 0x087f, 0x2727, 0x3333};
		const char *stl = " -|;:ji=", *s;
		const char *mrk = "*o+xsd.^v<>";
		const char *MRK = "YOPXSDCTVLR";
		const char *wdh = "123456789";
		const char *arr = "AKDTVISO_";
		long m=0;
		size_t l=strlen(p);
		for(size_t i=0;i<l;i++)
		{
			if(p[i]=='{')	m++;	if(p[i]=='}')	m--;
			if(m>0)	continue;
			s = mglchr(stl,p[i]);
			if(s)	{	PDef = val[s-stl];	last_style[4]=p[i];	}
			else if(mglchr(mrk,p[i]))	mk = p[i];
			else if(mglchr(wdh,p[i]))
			{	last_style[5] = p[i];	PenWidth = p[i]-'0';	}
			else if(mglchr(arr,p[i]))
			{
				if(!Arrow2)	Arrow2 = p[i];
				else	Arrow1 = p[i];
			}
		}
		if(Arrow1=='_')	Arrow1=0;	if(Arrow2=='_')	Arrow2=0;
		if(mglchr(p,'#'))
		{
			s = mglchr(mrk,mk);
			if(s)	mk = MRK[s-mrk];
		}
	}
	if(pal)
	{
		last_style[6]=mk;
		long tt, n;
		tt = AddTexture(p,-1);	n=Txt[tt].n;
		CDef = tt+((n+CurrPal-1)%n+0.5)/n;
		if(Id)	*Id=long(tt)*256+(n+CurrPal-1)%n;
	}
	return mk;
}
//-----------------------------------------------------------------------------
// keep this for restore default mask
uint64_t mgl_mask_def[16]={0x000000FF00000000,	0x080808FF08080808,	0x0000FF00FF000000,	0x0000007700000000,
							0x0000182424180000,	0x0000183C3C180000,	0x00003C24243C0000,	0x00003C3C3C3C0000,
							0x0000060990600000,	0x0060584658600000,	0x00061A621A060000,	0x0000005F00000000,
							0x0008142214080000,	0x00081C3E1C080000,	0x8142241818244281,	0x0000001824420000};
uint64_t mgl_mask_val[16]={0x000000FF00000000,	0x080808FF08080808,	0x0000FF00FF000000,	0x0000007700000000,
							0x0000182424180000,	0x0000183C3C180000,	0x00003C24243C0000,	0x00003C3C3C3C0000,
							0x0000060990600000,	0x0060584658600000,	0x00061A621A060000,	0x0000005F00000000,
							0x0008142214080000,	0x00081C3E1C080000,	0x8142241818244281,	0x0000001824420000};
void mglBase::SetMask(const char *p)
{
	mask = MGL_SOLID_MASK;	// reset to solid face
	PenWidth = 1;	MaskAn=DefMaskAn;
	if(p && *p)
	{
		const char *msk = MGL_MASK_ID, *s;
		const char *wdh = "123456789";
		long m=0, l=strlen(p);
		for(long i=0;i<l;i++)
		{
			if(p[i]=='{')	m++;	if(p[i]=='}')	m--;
			if(m>0)	continue;
			if(p[i]==':')	break;
			s = mglchr(msk, p[i]);
			if(s)	mask = mgl_mask_val[s-msk];
			else if(mglchr(wdh,p[i]))	PenWidth = p[i]-'0';
			else if(p[i]=='I')	MaskAn=90;
			else if(p[i]=='/')	MaskAn=315;	// =360-45
			else if(p[i]=='\\')	MaskAn=45;
		}
		// use line if rotation only specified
		if(mask==MGL_SOLID_MASK && MaskAn!=0)	mask = mgl_mask_val[0];
	}
}
//-----------------------------------------------------------------------------
mreal mglBase::GetA(mreal a) const
{
	if(fa)	a = fa->Calc(0,0,0,a);
	a = (a-FMin.c)/(FMax.c-FMin.c);
	a = (a<1?(a>0?a:0):1)/MGL_FEPSILON;	// for texture a must be <1 always!!!
	return a;
}
//-----------------------------------------------------------------------------
mglPoint GetX(HCDT x, int i, int j, int k)
{
	k = k<x->GetNz() ? k : 0;
	if(x->GetNy()>1)
		return mglPoint(x->v(i,j,k),x->dvx(i,j,k),x->dvy(i,j,k));
	else
		return mglPoint(x->v(i),x->dvx(i),0);
}
//-----------------------------------------------------------------------------
mglPoint GetY(HCDT y, int i, int j, int k)
{
	k = k<y->GetNz() ? k : 0;
	if(y->GetNy()>1)
		return mglPoint(y->v(i,j,k),y->dvx(i,j,k),y->dvy(i,j,k));
	else
		return mglPoint(y->v(j),0,y->dvx(j));
}
//-----------------------------------------------------------------------------
mglPoint GetZ(HCDT z, int i, int j, int k)
{
	if(z->GetNy()>1)
		return mglPoint(z->v(i,j,k),z->dvx(i,j,k),z->dvy(i,j,k));
	else
		return mglPoint(z->v(k),0,0);
}
//-----------------------------------------------------------------------------
void mglBase::vect_plot(long p1, long p2, mreal s)
{
	if(p1<0 || p2<0)	return;
	const mglPnt &q1=Pnt[p1], &q2=Pnt[p2];
	mglPnt s1=q2,s2=q2;
	s = s<=0 ? 0.1 : s*0.1;
	s1.x=s1.xx = q2.x - 3*s*(q2.x-q1.x) + s*(q2.y-q1.y);
	s2.x=s2.xx = q2.x - 3*s*(q2.x-q1.x) - s*(q2.y-q1.y);
	s1.y=s1.yy = q2.y - 3*s*(q2.y-q1.y) - s*(q2.x-q1.x);
	s2.y=s2.yy = q2.y - 3*s*(q2.y-q1.y) + s*(q2.x-q1.x);
	s1.z=s1.zz=s2.z=s2.zz = q2.z - 3*s*(q2.z-q1.z);
	long n1,n2;
#pragma omp critical(pnt)
	{
		n1=Pnt.size();	MGL_PUSH(Pnt,s1,mutexPnt);
		n2=Pnt.size();	MGL_PUSH(Pnt,s2,mutexPnt);
	}
	line_plot(p1,p2);	line_plot(n1,p2);	line_plot(p2,n2);
}
//-----------------------------------------------------------------------------
int mglFindArg(const char *str)
{
	long l=0,k=0,len=strlen(str);
	for(long i=0;i<len;i++)
	{
		if(str[i]=='\'') l++;
		if(str[i]=='{') k++;
		if(str[i]=='}') k--;
		if(l%2==0 && k==0)
		{
			if(str[i]=='#' || str[i]==';')	return -i;
			if(str[i]<=' ')	return i;
		}
	}
	return 0;
}
//-----------------------------------------------------------------------------
void mglBase::SetAmbient(mreal bright)	{	AmbBr = bright;	}
void mglBase::SetDiffuse(mreal bright)	{	DifBr = bright;	}
//-----------------------------------------------------------------------------
mreal mglBase::SaveState(const char *opt)
{
	if(saved)	return PrevState;
	if(!opt || !opt[0])	return NAN;
	MSS=MarkSize;	ASS=ArrowSize;
	FSS=FontSize;	ADS=AlphaDef;
	MNS=MeshNum;	CSS=Flag;	LSS=AmbBr;
	MinS=Min;		MaxS=Max;	saved=true;
	// parse option
	char *qi=mgl_strdup(opt),*q=qi, *s,*a,*b,*c;
	long n;
	mgl_strtrim(q);
	// NOTE: not consider '#' inside legend entry !!!
	s=(char*)strchr(q,'#');	if(s)	*s=0;
	mreal res=NAN;
	while(q && *q)
	{
    s = q;	q = (char*)strchr(s, ';');
		if(q)	{	*q=0;	q++;	}
		mgl_strtrim(s);		a=s;
		n=mglFindArg(s);	if(n>0)	{	s[n]=0;		s=s+n+1;	}
		mgl_strtrim(a);		b=s;
		n=mglFindArg(s);	if(n>0)	{	s[n]=0;		s=s+n+1;	}
		mgl_strtrim(b);

		mreal ff=atof(b),ss;
		if(!strcmp(b,"on"))	ff=1;
		if(!strcmp(a+1,"range"))
		{
			n=mglFindArg(s);	c=s;
			if(n>0)	{	s[n]=0;	s=s+n+1;	}
			mgl_strtrim(c);		ss = atof(c);
			if(a[0]=='x')		{	Min.x=ff;	Max.x=ss;	}
			else if(a[0]=='y')	{	Min.y=ff;	Max.y=ss;	}
			else if(a[0]=='z')	{	Min.z=ff;	Max.z=ss;	}
//			else if(a[0]=='c')	{	Min.c=ff;	Max.c=ss;	}	// Bad idea since there is formula for coloring
		}
		else if(!strcmp(a,"cut"))		SetCut(ff!=0);
		else if(!strcmp(a,"meshnum"))	SetMeshNum(ff);
		else if(!strcmp(a,"alpha"))		{Alpha(true);	SetAlphaDef(ff);}
		else if(!strcmp(a,"light"))		Light(ff!=0);
		else if(!strcmp(a,"ambient"))	SetAmbient(ff);
		else if(!strcmp(a,"diffuse"))	SetDifLight(ff);
		else if(!strcmp(a,"size"))
		{	SetMarkSize(ff);	SetFontSize(ff);	SetArrowSize(ff);	}
		else if(!strcmp(a,"num") || !strcmp(a,"number") || !strcmp(a,"value"))	res=ff;
		else if(!strcmp(a,"legend"))
		{	if(*b=='\'')	{	b++;	b[strlen(b)-1]=0;	}	leg_str = b;	}
	}
	free(qi);	PrevState=res;	return res;
}
//-----------------------------------------------------------------------------
void mglBase::LoadState()
{
	if(!saved)	return;
	MarkSize=MSS;	ArrowSize=ASS;
	FontSize=FSS;	AlphaDef=ADS;
	MeshNum=MNS;	Flag=CSS;	AmbBr=LSS;
	Min=MinS;		Max=MaxS;	saved=false;
}
//-----------------------------------------------------------------------------
void mglBase::AddLegend(const wchar_t *text,const char *style)
{
	if(text)
#pragma omp critical(leg)
		MGL_PUSH(Leg,mglText(text,style),mutexLeg);
}
//-----------------------------------------------------------------------------
void mglBase::AddLegend(const char *str,const char *style)
{
	MGL_TO_WCS(str,AddLegend(wcs, style));
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_dim2(HMGL gr, HCDT x, HCDT y, HCDT z, HCDT a, const char *name, bool less)
{
//	if(!gr || !x || !y || !z)	return true;		// if data is absent then should be segfault!!!
	register long n=z->GetNx(),m=z->GetNy();
	if(n<2 || m<2)	{	gr->SetWarn(mglWarnLow,name);	return true;	}
	if(a && n*m*z->GetNz()!=a->GetNx()*a->GetNy()*a->GetNz())
	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	if(less)
	{
		if(x->GetNx()<n)
		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(y->GetNx()<m && (x->GetNy()<m || y->GetNx()<n || y->GetNy()<m))
		{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	else
	{
		if(x->GetNx()!=n)
		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(y->GetNx()!=m && (x->GetNy()!=m || y->GetNx()!=n || y->GetNy()!=m))
		{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	return false;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_dim0(HMGL gr, HCDT x, HCDT y, HCDT z, HCDT r, const char *name, bool less)
{
//	if(!gr || !x || !y)	return true;		// if data is absent then should be segfault!!!
	register long n=y->GetNx();
	if(less)
	{
		if(x->GetNx()<n)		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(z && z->GetNx()<n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(r && r->GetNx()<n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	else
	{
		if(x->GetNx()!=n)		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(z && z->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(r && r->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	return false;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_dim1(HMGL gr, HCDT x, HCDT y, HCDT z, HCDT r, const char *name, bool less)
{
//	if(!gr || !x || !y)	return true;		// if data is absent then should be segfault!!!
	register long n=y->GetNx();
	if(n<2)	{	gr->SetWarn(mglWarnLow,name);	return true;	}
	if(less)
	{
		if(x->GetNx()<n)		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(z && z->GetNx()<n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(r && r->GetNx()<n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	else
	{
		if(x->GetNx()!=n)		{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(z && z->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
		if(r && r->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	}
	return false;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_dim3(HMGL gr, bool both, HCDT x, HCDT y, HCDT z, HCDT a, HCDT b, const char *name)
{
// 	if(!gr || !x || !y || !z || !a)	return true;		// if data is absent then should be segfault!!!
	register long n=a->GetNx(),m=a->GetNy(),l=a->GetNz();
	if(n<2 || m<2 || l<2)
	{	gr->SetWarn(mglWarnLow,name);	return true;	}
	if(!(both || (x->GetNx()==n && y->GetNx()==m && z->GetNx()==l)))
	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	if(b && b->GetNx()*b->GetNy()*b->GetNz()!=n*m*l)
	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	return false;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_trig(HMGL gr, HCDT nums, HCDT x, HCDT y, HCDT z, HCDT a, const char *name, int d)
{
// 	if(!gr || !x || !y || !z || !a || !nums)	return true;		// if data is absent then should be segfault!!!
	long n = x->GetNx(), m = nums->GetNy();
	if(nums->GetNx()<d)	{	gr->SetWarn(mglWarnLow,name);	return true;	}
	if(y->GetNx()!=n || z->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	if(a->GetNx()!=m && a->GetNx()!=n)	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	return false;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_isboth(HCDT x, HCDT y, HCDT z, HCDT a)
{
	register long n=a->GetNx(),m=a->GetNy(),l=a->GetNz();
	return x->GetNx()*x->GetNy()*x->GetNz()==n*m*l && y->GetNx()*y->GetNy()*y->GetNz()==n*m*l && z->GetNx()*z->GetNy()*z->GetNz()==n*m*l;
}
//-----------------------------------------------------------------------------
bool MGL_EXPORT mgl_check_vec3(HMGL gr, HCDT x, HCDT y, HCDT z, HCDT ax, HCDT ay, HCDT az, const char *name)
{
// 	if(!gr || !x || !y || !z || !ax || !ay || !az)	return true;		// if data is absent then should be segfault!!!
	register long n=ax->GetNx(),m=ax->GetNy(),l=ax->GetNz();
	if(n*m*l!=ay->GetNx()*ay->GetNy()*ay->GetNz() || n*m*l!=az->GetNx()*az->GetNy()*az->GetNz())
	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	if(n<2 || m<2 || l<2)	{	gr->SetWarn(mglWarnLow,name);	return true;	}
	bool both = x->GetNx()*x->GetNy()*x->GetNz()==n*m*l && y->GetNx()*y->GetNy()*y->GetNz()==n*m*l && z->GetNx()*z->GetNy()*z->GetNz()==n*m*l;
	if(!(both || (x->GetNx()==n && y->GetNx()==m && z->GetNx()==l)))
	{	gr->SetWarn(mglWarnDim,name);	return true;	}
	return false;
}
//-----------------------------------------------------------------------------
void mglBase::SetFontDef(const char *font) {	strncpy(FontDef, font, 31);	}
//-----------------------------------------------------------------------------
void mglBase::ClearUnused()
{
#if MGL_HAVE_PTHREAD
	pthread_mutex_lock(&mutexPnt);	pthread_mutex_lock(&mutexPrm);
#endif
#pragma omp critical
	{
		size_t l=Prm.size();
		// find points which are actually used
		long *used = new long[Pnt.size()];	memset(used,0,Pnt.size()*sizeof(long));
		for(size_t i=0;i<l;i++)
		{
			const mglPrim &p=Prm[i];
			if(p.n1<0)	continue;
			used[p.n1] = 1;
			switch(p.type)
			{
			case 1:	case 4:	if(p.n2>=0)	used[p.n2] = 1;	break;
			case 2:	if(p.n2>=0 && p.n3>=0)
				used[p.n2] = used[p.n3] = 1;	break;
			case 3:	if(p.n2>=0 && p.n3>=0 && p.n4>=0)
				used[p.n2] = used[p.n3] = used[p.n4] = 1;	break;
			}
		}
		// now add proper indexes
		l=Pnt.size();
		std::vector<mglPnt> pnt;
		for(size_t i=0;i<l;i++)	if(used[i])
		{	pnt.push_back(Pnt[i]);	used[i]=pnt.size();	}
		Pnt = pnt;	pnt.clear();
		// now replace point id
		l=Prm.size();
		for(size_t i=0;i<l;i++)
		{
			mglPrim &p=Prm[i];	p.n1=used[p.n1]-1;
			if(p.type==1 || p.type==4)	p.n2=used[p.n2]-1;
			if(p.type==2)	{	p.n2=used[p.n2]-1;	p.n3=used[p.n3]-1;	}
			if(p.type==3)	{	p.n2=used[p.n2]-1;	p.n3=used[p.n3]-1;	p.n4=used[p.n4]-1;	}
		}
		delete []used;
	}
#if MGL_HAVE_PTHREAD
	pthread_mutex_unlock(&mutexPnt);	pthread_mutex_unlock(&mutexPrm);
#endif
}
//-----------------------------------------------------------------------------
