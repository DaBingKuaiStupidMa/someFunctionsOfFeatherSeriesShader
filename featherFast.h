/* 今附MIT.license分享轻羽系列用到的个别强函数, 献自 @大冰块stupid吗 */
/* 正余弦近似:
* 误差在0.02以内, 周期为1.0 */
float cos2pi(highp float x){
	x=x-floor(x); x=0.5-abs(0.5-x);
	return x*x* (32.0*x-24.0) +1.0; // @大冰块stupid吗
}
#define sin2pi(x) cos2pi((x)-0.25)
/* atan(x,y)近似:
* 要求 (cosin, sine) 是严格的单位向量
* 返回 tetha(not theta) 在 [0,1){not[0,2pi)} 以内, 以便和cos2pi搭配
* 误差在0.38°以内 */
float getTetha(float cosin, float sine){ // 0.5/pi included
    float s2=abs(sine), tetha=abs(cosin);
    vec2 AixSin=( // applied to entire circle
        cosin>s2 ? vec2(0.0, sine) :(
        sine>=tetha ? vec2(0.25, -cosin) :(
        -cosin>s2 ? vec2(0.5, -sine) :vec2(0.75, cosin)
    ))); // use >= necessarily
    s2=AixSin.y *abs(AixSin.y);
    tetha=mix( // arcsin/(2pi)
        0.25 *s2, // 0.125/sqrt(0.5)^2
        0.1767766953 *AixSin.y, // 0.125/sqrt(0.5)
        0.8370726041 // found by PyScript
    )+AixSin.x; return tetha; // @大冰块stupid吗
}
/* 涟漪与水洼(纯程序):
* chunkXY 的数值在 [0,16)^2, h 取自 POSITION.y, 是0到15内的整数, t不限(可用TOTAL_REAL_WORLD_TIME)
* 返回涟漪向量 vec2 ripl 与水洼原料 float noise 合成的 vec3(ripl, noise)
    * ripl 的振幅最大为 1, 用法如 normal.xz +=riplNoise.xy *0.1919810
    * 涟漪按区块在空间上周期重复, 以 16s 周期在时间上重复
    * noise 是 [0,1) 之间的值噪声, 也依赖 lis 数组, 自行决定如何将它加工成水洼
    * 为了防止水洼过于细碎, noise 在区块上只有 8x8 个格点
* 时空随机性, 质量完全取决于几个 const float lis 数组, 如不满意, 设法另外生成
* 由于 lis 其实具有贴图的效用, 而且波形的计算涉及 sin2pi, 本人不知道与序列帧贴图相比性能孰好
* 如不引入波形序列帧贴图, "切勿试图优化" */
#define MACRO_addRip(_t0, t0_, xpy, xy, t0) {\
	fg=vec2(_t0,t0_)+vec2(xpy);\
	fg-=floor(fg);			tau=fg.x;\
	fg=chunkXY-(xy+fg);		r=dot(fg, fg);\
	_h=inversesqrt(r);		_h=min(256.0,_h);/*PE中等精度修瑕*/\
	vecR=fg*_h;				r*=_h;/*复用中间量*/\
	tau=t-(float(int(t0))+tau);\
	stren=max(0.0, 1.0-2.0*r);/*振幅衰减*/\
	r*=12.0; tau*=9.0;		wave=sin2pi(r+tau);\
	_h=r+tau-1.5;			_h=abs(_h)-0.5;\
	pack=max(0.0, 1.0-_h*_h);/*外扩波包*/\
	wave *=stren *pack;		ripl +=vecR *wave;/*手动标量优化*/\
} // _h as temp in addRip
#define f2u(f) uint(int(f))
#define _ /256.0
vec3 rainStuff(vec2 chunkXY, highp float t, uint h){ // h in [0, 16)
	const float lisX[16] = float[16](
		127.0 _, 203.0 _, 19.0 _, 88.0 _, 241.0 _, 66.0 _, 151.0 _, 32.0 _,
		99.0 _, 180.0 _, 234.0 _, 77.0 _, 14.0 _, 198.0 _, 61.0 _, 212.0 _
	);
	const float lisY[16] = float[16](
		191.0 _, 54.0 _, 228.0 _, 10.0 _, 163.0 _, 72.0 _, 135.0 _, 247.0 _,
		42.0 _, 186.0 _, 101.0 _, 223.0 _, 81.0 _, 159.0 _, 14.0 _, 238.0 _
	);
	const float _lisT[16] = float[16](
		77.0 _, 201.0 _, 254.0 _, 38.0 _, 123.0 _, 169.0 _, 88.0 _, 218.0 _,
		49.0 _, 133.0 _, 207.0 _, 62.0 _, 184.0 _, 96.0 _, 241.0 _, 15.0 _
	);
	const float lisT_[16] = float[16](
		163.0 _, 28.0 _, 206.0 _, 119.0 _, 244.0 _, 73.0 _, 198.0 _, 55.0 _,
		187.0 _, 141.0 _, 12.0 _, 229.0 _, 94.0 _, 178.0 _, 210.0 _, 67.0 _
	);
	vec2 xy=floor(chunkXY), UV=chunkXY-xy; // 坐标小数部分，切勿覆值
	vec2 fg, vecR, zw, ripl=vec2(0.0);
	zw.x =xy.x +(UV.x>0.5 ?1.0 :-1.0); // 涟漪有机会到达的邻格，切勿取模
	zw.y =xy.y +(UV.y>0.5 ?1.0 :-1.0);
	uint x, y, z, w, t1, t0, T1, T0;
	float lx, ly, lz, lw, _t1,t1_, _t0,t0_, _h, r, temp, tau, wave, stren, pack, noise=0.0;
	x=f2u(xy.x)&15u;	y=f2u(xy.y)&15u;	z=f2u(zw.x)&15u;	w=f2u(zw.y)&15u;
	t1=f2u(t);			t0=t1-1u;			T1=t1&15u;			T0=t0&15u; // 时空邻域
	lx=lisX[x];			ly=lisY[y];			lz=lisX[z];			lw=lisY[w];
	_t1=_lisT[T1];		t1_=lisT_[T1];		_t0=_lisT[T0];		t0_=lisT_[T0];
	temp=lx+ly; MACRO_addRip(_t0, t0_, temp, xy, t0) MACRO_addRip(_t1, t1_, temp, xy, t1)
	temp=lz+lw; MACRO_addRip(_t0, t0_, temp, zw, t0) MACRO_addRip(_t1, t1_, temp, zw, t1)
	temp=xy.y;	xy.y=zw.y;	zw.y=temp; // corner xy, zw done first, then xw, zy.
	temp=lx+lw; MACRO_addRip(_t0, t0_, temp, xy, t0) MACRO_addRip(_t1, t1_, temp, xy, t1)
	temp=lz+ly; MACRO_addRip(_t0, t0_, temp, zw, t0) MACRO_addRip(_t1, t1_, temp, zw, t1)
	#ifndef BLEND // z,w 的定义变为正向邻格
		fg=vec2(float(int(x&1u)), float(int(y&1u))); fg=(fg+UV)*0.5;
		x>>=1u;		y>>=1u;		z=(x+1u)&7u;	w=(y+1u)&7u;
		lx=lisX[x];	ly=lisY[y];	lz=lisX[z];		lw=lisY[w];		_h=_lisT[h];
		temp=_h+lw; _t1=temp+lx; _t1-=floor(_t1); t1_=temp+lz; t1_-=floor(t1_); // t1_ rytUp
		temp=_h+ly; _t0=temp+lx; _t0-=floor(_t0); t0_=temp+lz; t0_-=floor(t0_); //_t0 lefDown
		temp =mix(_t1, t1_, fg.x);
		noise=mix(_t0, t0_, fg.x); noise=mix(noise, temp, fg.y); // noise in [0,1)
	#endif
	return vec3(ripl, noise); // @大冰块stupid吗
}
#undef _
/* 在太阳月亮上刻纹:
* 忠告! 本方案封装程度差, 对几何有考验, 如果非要适配, 应借助AI
* 16x16 像素图案, 每个像素只有两个状态, 0 是默认状态, 用 1 标记刻纹
* 大冰块能力有限, 务必不要将最外圈改成 1, 否则异常
* drawMoon 和 drawSun 使用的 uv 在 [-1,1)^2 以内
* drawSquare 用到的变量中
    * sunAt 是真正太阳的归一化方向(可转到地平线以下), 决不是网易默认提供的 SUN_DIR.xyz(取地平线以上的一方)
    * ePHI 和 eTHT 是外部的全局归一化 vec3, 描述了天球上 sunAt 附近的平面正交系, 好似经纬线
        * 是的, 这要求您结合自己 sunAt 的实际设计, 以及线性代数的知识, 自行确认 ePHI 和 eTHT
    * 特别注意, 轻羽系列的太阳和月亮总是关于视点(摄像机)中心对称, 所以 ePHI 和 eTHT 是公用的
        * 是的, 这意味你很可能要为自己的光影计算两套 ePHI 和 eTHT
    * viewDir 当然是归一化的视线方向, 对于屏幕上每个像素, viewDir的数值都是独特的
* useFade 通过 *alpha 削弱太阳和月亮, 例如晴雨因子
* 如你使用高版本 GLSL, 大可用 0b 开头的 binary 字面量取代依赖宏的位运算写法
    * 定义32左括宏是为压代码
    * 定义31右括宏是为阻止编辑器中, 文本染色或语法校验插件, 因括号不匹配将下文染红 */
#define leftBracket32 (((( (((( (((( (((( (((( (((( (((( ((((
#define rytBracket31 )))) )))) )))) )))) )))) )))) )))) )))
#define _ )<<1
#define MACRO_Moon (int[8](\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|1_|1_|1_ |1_|1_|1_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|1_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |1_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|1_ |0_|0_|0_|0_ |0_|1_|1_|1_ |1_|1_|0_|0_\
                    |0_|0_|1_|1_ |1_|1_|1_|0_ |0_|0_|0_|0_ |1_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|1_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|1_|1_ |1_|1_|1_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|1_,\
    leftBracket32 0 |0_|1_|1_|1_ |1_|0_|0_|0_ |1_|1_|1_|1_ |1_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|1_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|1_|1_ |1_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
))
#define MACRO_Sun (int[8](\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |1_|1_|0_|0_ |0_|0_|1_|1_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|1_|1_ |1_|1_|0_|0_ |0_|1_|1_|1_ |1_|1_|0_|0_\
                    |0_|1_|0_|0_ |1_|1_|0_|0_ |0_|0_|1_|1_ |0_|0_|1_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |1_|1_|0_|0_ |0_|0_|1_|1_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |1_|1_|0_|0_ |0_|0_|1_|1_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|1_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|1_ |1_|1_|0_|0_ |0_|0_|0_|0_,\
    leftBracket32 0 |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
                    |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_ |0_|0_|0_|0_\
))
vec4 drawMoon(vec2 uv){
    vec2 xy=abs(uv); highp float alpha=max(xy.x, xy.y);
    alpha=(clamp(alpha,0.5,0.8)-0.5) *3.33334; // 4 as the end necessarily
    alpha=1.0-floor(alpha*4.0) *0.25; // 实际是三段透

    vec3 moonColor=vec3(0.8,0.9,0.9);
    const highp int txMoon[8] =MACRO_Moon;

    xy=uv +vec2(0.5);
    xy=(xy *16.0); // 出于未知原因，16而不是15出于经验
    int y=(15-int(xy.y)); int x=(16-int(xy.x));
    if(!(x>15 || x<0 || y>15 || y<0)){
        highp int row=txMoon[y>>1];
        row=(y&1)>0 ?row :(row>>16);
        x=int((row>>x)&1);
    }else x=0;
    if(x>0) moonColor*=0.85;
    return vec4(moonColor, alpha); // @大冰块stupid吗
}
vec4 drawSun(vec2 uv){
    vec2 xy=abs(uv); highp float alpha=max(xy.x, xy.y);
    alpha=(clamp(alpha,0.5,1.0)-0.5) *2.0;
    alpha=1.0-floor(alpha*5.0) *0.2; // 实际是四段透

    vec3 sunColor=vec3(1.0,1.0,0.95);
    const highp int txSun[8] =MACRO_Sun;

    xy=uv +vec2(0.6);
    xy=(xy *(40.0/3.0)); // 出于未知原因，16而不是15出于经验
    int y=int(xy.y); int x=(16-int(xy.x));
    if(!(x>15 || x<0 || y>15 || y<0)){
        highp int row=txSun[y>>1];
        row=(y&1)>0 ?row :(row>>16);
        x=int((row>>x)&1);
    }else x=0;
    if(x>0) sunColor*=0.85;
    return vec4(sunColor, alpha); // @大冰块stupid吗
}
#undef _
vec4 drawSquares(vec3 viewDir, vec3 sunAt, vec2 useFade){
    float VdS=dot(viewDir, sunAt);
    vec4 color=vec4(0.0);
    if(abs(VdS)>0.9){
		vec3 deltaV =viewDir -sunAt;
		deltaV =deltaV -dot(deltaV, sunAt) *sunAt; // 施密特正交化
		vec2 sunUV;
        sunUV.x=dot(ePHI, deltaV);
        sunUV.y=dot(eTHT, deltaV); sunUV*=10.0;
		if(abs(sunUV.x)<1.0 && abs(sunUV.y)<1.0){
            if(VdS>0.0) {color=drawSun(sunUV); color.a*=useFade.x;}
            else        {color=drawMoon(sunUV);color.a*=useFade.y;}
        }
    }return color; // @大冰块stupid吗
}
