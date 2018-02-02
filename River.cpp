#include <osg/ShapeDrawable>  
#include <osg/Geometry>  
#include <osgGA/TrackballManipulator>  
#include <osg/Texture2D>  
#include <osg/Geode>  
#include <osg/TextureCubeMap>

#include <osgDB/ReadFile>  
#include <osgDB/WriteFile>  

#include <osgViewer/Viewer>  
#include <osgViewer/ViewerEventHandlers>
#include <string>  

#ifdef _DEBUG
#pragma comment(lib, "osgViewerd.lib")
#pragma comment(lib, "osgd.lib")
#pragma comment(lib, "osgDBd.lib")
#pragma comment(lib, "osgUtild.lib")
#pragma comment(lib, "osgGAd.lib")
#else
#pragma comment(lib, "osgViewer.lib")
#pragma comment(lib, "osg.lib")
#pragma comment(lib, "osgDB.lib")
#pragma comment(lib, "osgUtil.lib")
#pragma comment(lib, "osgGA.lib")
#endif // _DEBUG

using namespace std;  

static char* river_vert = 
{
// Vertex shader 
"varying vec3  Normal;   \n"
"varying vec3  EyeDir;   \n"
"   \n"
"varying vec2 texNormal0Coord;   \n"
"varying vec2 texColorCoord;   \n"
"varying vec2 texFlowCoord;   \n"
"   \n"
"varying vec3 vertex;   \n"
"   \n"
"uniform float osg_FrameTime;   \n"
"uniform mat4  osg_ViewMatrixInverse;   \n"
"varying float myTime;   \n"
"   \n"
"void main(void)   \n"
"{   \n"
"gl_Position     = ftransform();   \n"
"Normal          = normalize(gl_NormalMatrix * gl_Normal);   \n"
"   \n"
"vec4 pos        = gl_ModelViewMatrix * gl_Vertex;   \n"
"EyeDir          = vec3(osg_ViewMatrixInverse*vec4(pos.xyz,0));   \n"
"   \n"
"texNormal0Coord = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;   \n"
"texFlowCoord    = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;   \n"
"texColorCoord   = (gl_TextureMatrix[0] * gl_MultiTexCoord0).xy;   \n"
"   \n"
"myTime = 0.01 * osg_FrameTime;   \n"
"   \n"
"vertex = gl_Vertex.xyz;   \n"
"}   \n"
};

static char* river_frag = 
{
"uniform sampler2D   normalMap;  \n"
"  \n"
"uniform sampler2D   colorMap; \n"
"uniform sampler2D   flowMap; \n"
"uniform samplerCube cubeMap; \n"
"  \n"
"varying vec3  Normal; \n"
"varying vec3  EyeDir; \n"
"  \n"
"varying vec2 texNormal0Coord; \n"
"varying vec2 texColorCoord; \n"
"varying vec2 texFlowCoord; \n"
"  \n"
"varying vec3 vertex; \n"
"  \n"
"varying float myTime; \n"
"  \n"
"void main (void) \n"
"{  \n"
//"texScale determines the amount of tiles generated.  \n"
"float texScale = 35.0; \n"
"  \n"
//"texScale2 determines the repeat of the water texture (the normalmap) itself  \n"
"float texScale2 = 10.0; \n"
"float myangle;  \n"
"float transp; \n"
"vec3  myNormal;  \n"
"  \n"
"vec2 mytexFlowCoord = texFlowCoord * texScale; \n"
"  \n"
//" ff is the factor that blends the tiles.  \n"
"vec2 ff =  abs(2*(frac(mytexFlowCoord)) - 1) -0.5;  \n"
"  \n"
//" take a third power, to make the area with more or less equal contribution  \n"
//" of more tile bigger \n"
"ff = 0.5-4*ff*ff*ff; \n"
"  \n"
//" ffscale is a scaling factor that compensates for the effect that  \n"
//" adding normal vectors together tends to get them closer to the average normal  \n"
//" which is a visible effect. For more or less random waves, this factor  \n"
//" compensates for it  \n"
"vec2 ffscale = sqrt(ff*ff + (1-ff)*(1-ff));  \n"
"vec2 Tcoord  = texNormal0Coord  * texScale2;  \n"
"  \n"
//" offset makes the water move \n"
"vec2 offset = vec2(myTime,0); \n"
"  \n"
//" I scale the texFlowCoord and floor the value to create the tiling  \n"
//" This could have be replace by an extremely lo-res texture lookup  \n"
//" using NEAREST pixel. \n"
"vec3 sample = texture2D( flowMap, floor(mytexFlowCoord)/ texScale).rgb;  \n"
"  \n"
//" flowdir is supposed to go from -1 to 1 and the line below  \n"
//" used to be sample.xy * 2.0 - 1.0, but saves a multiply by  \n"
//" moving this factor two to the sample.b \n"
"vec2 flowdir = sample.xy -0.5;      \n"
"  \n"
//" sample.b is used for the inverse length of the wave  \n"
//" could be premultiplied in sample.xy, but this is easier for editing flowtexture  \n"
"flowdir *= sample.b; \n"
"  \n"
//" build the rotation matrix that scales and rotates the complete tile  \n"
"mat2 rotmat = mat2(flowdir.x, -flowdir.y, flowdir.y ,flowdir.x);  \n"
"  \n"
//" this is the normal for tile A  \n"
"vec2 NormalT0 = texture2D(normalMap, rotmat * Tcoord - offset).rg;  \n"
"  \n"
//" for the next tile (B) I shift by half the tile size in the x-direction  \n"
"sample  = texture2D( flowMap, floor((mytexFlowCoord + vec2(0.5,0)))/ texScale ).rgb;  \n"
"flowdir = sample.b * (sample.xy - 0.5);  \n"
"rotmat  = mat2(flowdir.x, -flowdir.y, flowdir.y ,flowdir.x);  \n"
"  \n"
//" and the normal for tile B...  \n"
//" multiply the offset by some number close to 1 to give it a different speed  \n"
//" The result is that after blending the water starts to animate and look  \n"
//" realistic, instead of just sliding in some direction.  \n"
//" This is also why I took the third power of ff above, so the area where the  \n"
//" water animates is as big as possible  \n"
//" adding a small arbitrary constant isn't really needed, but helps to show  \n"
//" a bit less tiling in the beginning of the program. After a few seconds, the  \n"
//" tiling cannot be seen anymore so this constant could be removed.  \n"
//" For the quick demo I leave them in. In a simulation that keeps running for  \n"
//" some time, you could just as well remove these small constant offsets  \n"
"vec2 NormalT1 = texture2D(normalMap, rotmat * Tcoord - offset*1.06+0.62).rg ;  \n"
"  \n"
//" blend them together using the ff factor  \n"
//" use ff.x because this tile is shifted in the x-direction   \n"
"vec2 NormalTAB = ff.x * NormalT0 + (1-ff.x) * NormalT1;  \n"
"  \n"
//" the scaling of NormalTab and NormalTCD is moved to a single scale of  \n"
//" NormalT later in the program, which is mathematically identical to  \n"
//" NormalTAB = (NormalTAB - 0.5) / ffscale.x + 0.5;  \n"
"  \n"
//" tile C is shifted in the y-direction   \n"
"sample = texture2D( flowMap, floor((mytexFlowCoord + vec2(0.0,0.5)))/ texScale ).rgb;  \n"
"flowdir = sample.b * (sample.xy - 0.5);  \n"
"rotmat = mat2(flowdir.x, -flowdir.y, flowdir.y ,flowdir.x);  \n"
"NormalT0 = texture2D(normalMap, rotmat * Tcoord - offset*1.33+0.27).rg;  \n"
"  \n"
//" tile D is shifted in both x- and y-direction  \n"
"sample = texture2D( flowMap, floor((mytexFlowCoord + vec2(0.5,0.5)))/ texScale ).rgb;  \n"
"flowdir = sample.b * (sample.xy - 0.5);  \n"
"rotmat = mat2(flowdir.x, -flowdir.y, flowdir.y ,flowdir.x);  \n"
"NormalT1 = texture2D(normalMap, rotmat * Tcoord - offset*1.24).rg;  \n"
"  \n"
"vec2 NormalTCD = ff.x * NormalT0 + (1-ff.x) * NormalT1;  \n"
"NormalTCD = (NormalTCD - 0.5) / ffscale.x + 0.5; \n"
"  \n"
//" now blend the two values together \n"
"vec2 NormalT = ff.y * NormalTAB + (1-ff.y) * NormalTCD;  \n"
"  \n"
//" this line below used to be here for scaling the result  \n"
//"NormalT = (NormalT - 0.5) / ffscale.y + 0.5;  \n"
"  \n"
//" below the new, direct scaling of NormalT  \n"
"NormalT = (NormalT - 0.5) / (ffscale.y * ffscale.x);  \n"
"  \n"
//" scaling by 0.3 is arbritrary, and could be done by just  \n"
//" changing the values in the normal map \n"
//" without this factor, the waves look very strong  \n"
"NormalT *= 1.0;  \n"
"  \n"
//" to make the water more transparent \n"
"transp = texture2D( flowMap, texFlowCoord ).a;  \n"
"  \n"
//"if(transp >0.0f) \n"
//"{  \n"
//"vertex.y += 100.0f; \n"
//"}  \n"
"  \n"
//" and scale the normals with the transparency  \n"
"NormalT *= transp*transp; \n"
"  \n"
//" assume normal of plane is 0,0,1 and produce the normalized sum of adding NormalT to it  \n"
"myNormal = vec3(NormalT,sqrt(1-NormalT.x*NormalT.x - NormalT.y*NormalT.y));  \n"
"  \n"
"vec3 reflectDir = reflect(EyeDir, myNormal);  \n"
"vec3 envColor   = vec3 (textureCube(cubeMap, -reflectDir));   \n"
"  \n"
//" very ugly version of fresnel effect  \n"
//" but it gives a nice transparent water, but not too transparent  \n"
"myangle = dot(myNormal,normalize(EyeDir)); \n"
"myangle = 0.95-0.6*myangle*myangle;  \n"
"  \n"
//" blend in the color of the plane below the water  \n"
"  \n"
//" add in a little distortion of the colormap for the effect of a refracted  \n"
//" view of the image below the surface.   \n"
//" (this isn't really tested, just a last minute addition  \n"
//" and perhaps should be coded differently  \n"
"  \n"
//" the correct way, would be to use the refract routine, use the alpha channel for depth of   \n"
//" the water (and make the water disappear when depth = 0), add some watercolor to the colormap  \n"
//" depending on the depth, and use the calculated refractdir and the depth to find the right  \n"
//" pixel in the colormap.... who knows, something for the next version  \n"
"vec3 base = texture2D(colorMap,texColorCoord + myNormal/texScale2*0.03*transp).rgb;  \n"
//"vec3 base = texture2D(colorMap,texColorCoord).rgb; \n"
"gl_FragColor = vec4 (base, 1.0);  \n"
"  \n"
"gl_FragColor = vec4 (mix(base,envColor,myangle*transp),1.0 );  \n"
"  \n"
//" note that smaller waves appear to move slower than bigger waves  \n"
//" one could use the tiles and give each tile a different speed if that  \n"
//" is what you want  \n"
"}  \n"
};


//2d map
osg::Texture2D* createTexture( const std::string& filename )
{
osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
texture->setImage( osgDB::readImageFile(filename) );
texture->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT );
texture->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT );
texture->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR );
texture->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );
return texture.release();
}

//cube map
osg::ref_ptr<osg::TextureCubeMap> creatCubeMap()
{
osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;

osg::ref_ptr<osg::Image> imagePosX = osgDB::readImageFile("n_x.tga");
osg::ref_ptr<osg::Image> imageNegX = osgDB::readImageFile("n_y.tga");
osg::ref_ptr<osg::Image> imagePosY = osgDB::readImageFile("n_z.tga");
osg::ref_ptr<osg::Image> imageNegY = osgDB::readImageFile("p_x.tga");
osg::ref_ptr<osg::Image> imagePosZ = osgDB::readImageFile("p_y.tga");
osg::ref_ptr<osg::Image> imageNegZ = osgDB::readImageFile("p_z.tga");

if (imagePosX.get() && imageNegX.get() && imagePosY.get() && imageNegY.get() && imagePosZ.get() && imageNegZ.get())
{
//设置立方图的六个面的贴图;
cubemap->setImage(osg::TextureCubeMap::POSITIVE_X, imagePosX.get());
cubemap->setImage(osg::TextureCubeMap::NEGATIVE_X, imageNegX.get());
cubemap->setImage(osg::TextureCubeMap::POSITIVE_Y, imagePosY.get());
cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Y, imageNegY.get());
cubemap->setImage(osg::TextureCubeMap::POSITIVE_Z, imagePosZ.get());
cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Z, imageNegZ.get());

//设置纹理环绕模式;
cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

//设置滤波：线形和mipmap;
cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
}

return cubemap.get();
}

//quad
osg::Geode* createScreenQuad( float width, float height, float scale )
{
osg::Geometry* geom = osg::createTexturedQuadGeometry(
osg::Vec3(), osg::Vec3(width,0.0f,0.0f), osg::Vec3(0.0f,height,0.0f),
0.0f, 0.0f, width*scale, height*scale );
osg::ref_ptr<osg::Geode> quad = new osg::Geode;
quad->addDrawable( geom );

return quad.release();
}

int main(int argc, char ** argv)
{
osgViewer::Viewer viewer;
viewer.setUpViewInWindow(200,50,800,600);

osg::Group* root = new osg::Group;
osg::Node* node = createScreenQuad(1. , 1. , 1.0);
osg::Node* node1 = osgDB::readNodeFile("terr.osgb");

osg::ref_ptr<osg::Shader> vertShader = new osg::Shader(osg::Shader::VERTEX, river_vert);
osg::ref_ptr<osg::Shader> fragShader = new osg::Shader(osg::Shader::FRAGMENT, river_frag);
osg::ref_ptr<osg::Program> program = new osg::Program;
program->addShader( vertShader.get() );
program->addShader( fragShader.get() );

node->getOrCreateStateSet()->setTextureAttributeAndModes( 0, createTexture("w1-2-grey.tga") );
node->getOrCreateStateSet()->setTextureAttributeAndModes( 1, createTexture("hpcvrock.tga") );
node->getOrCreateStateSet()->setTextureAttributeAndModes( 2, createTexture("hpcvwater.tga") );
node->getOrCreateStateSet()->setTextureAttributeAndModes( 3, creatCubeMap() );

node->getOrCreateStateSet()->addUniform( new osg::Uniform("normalMap", 0) );
node->getOrCreateStateSet()->addUniform( new osg::Uniform("colorMap" , 1) );
node->getOrCreateStateSet()->addUniform( new osg::Uniform("flowMap"  , 2) );
node->getOrCreateStateSet()->addUniform( new osg::Uniform("cubeMap"  , 3) );

node->getOrCreateStateSet()->setAttributeAndModes( program.get() );

osg::Vec3d center_ = node->getBound().center() + osg::Vec3d(-0.2, 0.0, 0.0);
osg::Vec3d eye_    = center_ + osg::Vec3d(0.0, -0.5, 0.5)*0.5;
osg::ref_ptr<osgGA::TrackballManipulator> tm = new osgGA::TrackballManipulator;
tm->setHomePosition(eye_, center_, osg::Z_AXIS);
viewer.setCameraManipulator(tm);


root->addChild(node);
//root->addChild(node1);

viewer.setSceneData(root);
viewer.addEventHandler(new osgViewer::WindowSizeHandler);
viewer.addEventHandler(new osgViewer::StatsHandler);
viewer.run();

return 0;
}