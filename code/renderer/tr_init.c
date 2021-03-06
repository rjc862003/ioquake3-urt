/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"
#include "../ioq3-urt/ioq3-urt.h"
extern cvar_t *com_quiet;
glconfig_t      glConfig;
qboolean        textureFilterAnisotropic = qfalse;
int             maxAnisotropy = 0;
float       displayAspect = 0.0f;
qboolean    vertexBufferObjects = qfalse;
qboolean    vertexShaders = qfalse;

glstate_t       glState;

static void GfxInfo_f( void );
static void GlslInfo_f( void );

cvar_t  *r_flareSize;
cvar_t  *r_flareFade;
cvar_t  *r_flareCoeff;

cvar_t  *r_railWidth;
cvar_t  *r_railCoreWidth;
cvar_t  *r_railSegmentLength;

cvar_t  *r_ignoreFastPath;

cvar_t  *r_verbose;
cvar_t  *r_ignore;

cvar_t  *r_detailTextures;

cvar_t  *r_znear;
cvar_t  *r_zproj;
cvar_t  *r_stereoSeparation;

cvar_t  *r_smp;
cvar_t  *r_showSmp;
cvar_t  *r_skipBackEnd;

cvar_t  *r_stereoEnabled;
cvar_t  *r_anaglyphMode;

cvar_t  *r_greyscale;

cvar_t  *r_ignorehwgamma;
cvar_t  *r_measureOverdraw;

cvar_t  *r_inGameVideo;
cvar_t  *r_fastsky;
cvar_t  *r_drawSun;
cvar_t  *r_dynamiclight;
cvar_t  *r_dlightBacks;
cvar_t  *r_minEntityLight;

cvar_t  *r_specularLighting;
cvar_t  *r_specularLightingExponent;

cvar_t  *r_lodbias;
cvar_t  *r_lodscale;

cvar_t  *r_norefresh;
cvar_t  *r_drawentities;
cvar_t  *r_drawworld;
cvar_t  *r_speeds;
cvar_t  *r_fullbright;
cvar_t  *r_novis;
cvar_t  *r_nocull;
cvar_t  *r_facePlaneCull;
cvar_t  *r_showcluster;
cvar_t  *r_nocurves;

cvar_t  *r_allowExtensions;

cvar_t  *r_ext_compressed_textures;
cvar_t  *r_ext_multitexture;
cvar_t  *r_ext_compiled_vertex_array;
cvar_t  *r_ext_texture_env_add;
cvar_t  *r_ext_texture_filter_anisotropic;
cvar_t  *r_ext_max_anisotropy;
cvar_t  *r_ext_vertex_buffer_object;
cvar_t  *r_ext_framebuffer_object;
cvar_t  *r_ext_vertex_shader;

cvar_t  *r_ignoreGLErrors;
cvar_t  *r_logFile;

cvar_t  *r_stencilbits;
cvar_t  *r_depthbits;
cvar_t  *r_colorbits;
cvar_t  *r_primitives;
cvar_t  *r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t  *r_drawBuffer;
cvar_t  *r_lightmap;
cvar_t  *r_vertexLight;
cvar_t  *r_uiFullScreen;
cvar_t  *r_shadows;
cvar_t  *r_flares;
cvar_t  *r_mode;
cvar_t  *r_nobind;
cvar_t  *r_singleShader;
cvar_t  *r_roundImagesDown;
cvar_t  *r_colorMipLevels;
cvar_t  *r_picmip;
cvar_t  *r_showtris;
cvar_t  *r_showsky;
cvar_t  *r_shownormals;
cvar_t  *r_finish;
cvar_t  *r_clear;
cvar_t  *r_swapInterval;
cvar_t  *r_textureMode;
cvar_t  *r_offsetFactor;
cvar_t  *r_offsetUnits;
cvar_t  *r_gamma;
cvar_t  *r_intensity;
cvar_t  *r_lockpvs;
cvar_t  *r_noportals;
cvar_t  *r_portalOnly;

cvar_t  *r_subdivisions;
cvar_t  *r_lodCurveError;

cvar_t  *r_fullscreen;
cvar_t  *r_noborder;
cvar_t  *r_minimize;

cvar_t  *r_width;
cvar_t  *r_height;
cvar_t  *r_pixelAspect;

cvar_t  *r_overBrightBits;
cvar_t  *r_mapOverBrightBits;

cvar_t  *r_debugSurface;
cvar_t  *r_simpleMipMaps;

cvar_t  *r_showImages;

cvar_t  *r_ambientScale;

cvar_t  *r_decals;
cvar_t  *r_maxstaticverts;
cvar_t  *r_bump;
cvar_t  *r_postwidth;
cvar_t  *r_postflt;
cvar_t  *r_postadd;

cvar_t  *r_post_gamma;
cvar_t  *r_post_brightness;
cvar_t  *r_post_contrast;
cvar_t  *r_post_lightness;
cvar_t  *r_post_vibrance;
cvar_t  *r_post_saturation;

cvar_t  *r_bloom;
cvar_t  *r_bloomcut;
cvar_t  *r_bloomadd;
cvar_t  *r_bloomflt;
cvar_t  *r_bloomfltwidth;
cvar_t  *r_bloomres;
cvar_t  *r_autobump;
cvar_t  *r_dof;
cvar_t  *r_dofscale;
cvar_t  *r_dofblend;
cvar_t  *r_fancywater;
cvar_t  *r_simpleshaders;
cvar_t  *r_glsl_debug;

cvar_t  *r_fogdensity;
cvar_t  *r_fog_r;
cvar_t  *r_fog_g;
cvar_t  *r_fog_b;

cvar_t  *r_directedScale;
cvar_t  *r_debugLight;
cvar_t  *r_debugSort;
cvar_t  *r_printShaders;
cvar_t  *r_saveFontData;

cvar_t *r_marksOnTriangleMeshes;
cvar_t  *r_GLlibCoolDownMsec;

cvar_t  *r_maxpolys;
int             max_polys;
cvar_t  *r_maxpolyverts;
int             max_polyverts;

static void AssertCvarRange( cvar_t *cv, float minVal, float maxVal, qboolean shouldBeIntegral )
{
        if ( shouldBeIntegral )
        {
                if ( ( int ) cv->value != cv->integer )
                {
                        ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
                        ri.Cvar_Set( cv->name, va( "%d", cv->integer ) );
                }
        }

        if ( cv->value < minVal )
        {
                ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
                ri.Cvar_Set( cv->name, va( "%f", minVal ) );
        }
        else if ( cv->value > maxVal )
        {
                ri.Printf( PRINT_WARNING, "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
                ri.Cvar_Set( cv->name, va( "%f", maxVal ) );
        }
}


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
        char renderer_buffer[1024];

        //
        // initialize OS specific portions of the renderer
        //
        // GLimp_Init directly or indirectly references the following cvars:
        //              - r_fullscreen
        //              - r_(width|height|pixelAspect)
        //              - r_(color|depth|stencil)bits
        //              - r_ignorehwgamma
        //              - r_gamma
        //

        if ( glConfig.vidWidth == 0 )
        {
                GLint           temp;

                GLimp_Init();

                strcpy( renderer_buffer, glConfig.renderer_string );
                Q_strlwr( renderer_buffer );

                // OpenGL driver constants
                qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
                glConfig.maxTextureSize = temp;

                // stubbed or broken drivers may have reported 0...
                if ( glConfig.maxTextureSize <= 0 )
                {
                        glConfig.maxTextureSize = 0;
                }
        }

        // init command buffers and SMP
        R_InitCommandBuffers();

        // init GLSL shaders
        R_LoadSimpleFragmentPrograms( );
        R_LoadFragmentPrograms( );
        R_LoadSM3FragmentPrograms( );

        // print info
        if (!com_quiet->integer)
                GfxInfo_f();

        // set default state
        GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
    int         err;
    char        s[64];

    err = qglGetError();
    if ( err == GL_NO_ERROR ) {
        return;
    }
    if ( r_ignoreGLErrors->integer ) {
        return;
    }
    switch( err ) {
        case GL_INVALID_ENUM:
            strcpy( s, "GL_INVALID_ENUM" );
            break;
        case GL_INVALID_VALUE:
            strcpy( s, "GL_INVALID_VALUE" );
            break;
        case GL_INVALID_OPERATION:
            strcpy( s, "GL_INVALID_OPERATION" );
            break;
        case GL_STACK_OVERFLOW:
            strcpy( s, "GL_STACK_OVERFLOW" );
            break;
        case GL_STACK_UNDERFLOW:
            strcpy( s, "GL_STACK_UNDERFLOW" );
            break;
        case GL_OUT_OF_MEMORY:
            strcpy( s, "GL_OUT_OF_MEMORY" );
            break;
        default:
            Com_sprintf( s, sizeof(s), "%i", err);
            break;
    }

    ri.Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
        float           pixelAspect;            // pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] =
{
    { "Mode  0: 320x240",               320,    240,    1 },
    { "Mode  1: 400x300",               400,    300,    1 },
    { "Mode  2: 512x384",               512,    384,    1 },
    { "Mode  3: 640x480",               640,    480,    1 },
    { "Mode  4: 800x600",               800,    600,    1 },
    { "Mode  5: 960x720",               960,    720,    1 },
    { "Mode  6: 1024x768",              1024,   768,    1 },
    { "Mode  7: 1152x864",              1152,   864,    1 },
    { "Mode  8: 1280x1024",             1280,   1024,   1 },
    { "Mode  9: 1600x1200",             1600,   1200,   1 },
    { "Mode 10: 2048x1536",             2048,   1536,   1 },
    { "Mode 11: 856x480 (wide)",        856,    480,    1 },
    { "Mode 12: 1280x720 (wide)",       1280,   720,    1 },
    { "Mode 13: 1368x768 (wide)",       1368,   768,    1 },
    { "Mode 14: 1600x900 (wide)",       1600,   900,    1 },
    { "Mode 15: 1680x1050 (wide)",      1680,   1050,   1 },
    { "Mode 16: 1920x1080 (wide)",      1920,   1080,   1 },
    { "Mode 17: 2560x1440 (wide)",      2560,   1440,   1 }
};
static int      s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode ) {
        vidmode_t        *vm;
        float                   pixelAspect;

    if ( mode < -1 ) {
        return qfalse;
        }
        if ( mode >= s_numVidModes ) {
                return qfalse;
        }

        if ( mode == -1 ) {
                *width = r_width->integer;
                *height = r_height->integer;
                pixelAspect = r_pixelAspect->value;
        }       else {
                vm = &r_vidModes[mode];

                *width  = vm->width;
                *height = vm->height;
                pixelAspect = vm->pixelAspect;
        }

        *windowAspect = (float)*width / ( *height * pixelAspect );

        return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
        int i;

        ri.Printf( PRINT_ALL, "\n" );
        for ( i = 0; i < s_numVidModes; i++ )
        {
                ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
        }
        ri.Printf( PRINT_ALL, "\n" );
}


/*
==============================================================================

                                                SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
        byte            *buffer;
        int                     i, c, temp;

        buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3+18);

        Com_Memset (buffer, 0, 18);
        buffer[2] = 2;          // uncompressed type
        buffer[12] = width & 255;
        buffer[13] = width >> 8;
        buffer[14] = height & 255;
        buffer[15] = height >> 8;
        buffer[16] = 24;        // pixel size

        qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

        // swap rgb to bgr
        c = 18 + width * height * 3;
        for (i=18 ; i<c ; i+=3) {
                temp = buffer[i];
                buffer[i] = buffer[i+2];
                buffer[i+2] = temp;
        }

        // gamma correct
        if ( glConfig.deviceSupportsGamma ) {
                R_GammaCorrect( buffer + 18, glConfig.vidWidth * glConfig.vidHeight * 3 );
        }

        ri.FS_WriteFile( fileName, buffer, c );

        ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/
void RB_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
        byte            *buffer;

        buffer = ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*4);

        qglReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer );

        // gamma correct
        if ( glConfig.deviceSupportsGamma ) {
                R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 4 );
        }

        ri.FS_WriteFile( fileName, buffer, 1 );         // create path
        SaveJPG( fileName, clu.r_screenshotJPEGQuality->integer, glConfig.vidWidth, glConfig.vidHeight, buffer);

        ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
        const screenshotCommand_t       *cmd;

        cmd = (const screenshotCommand_t *)data;

        if (cmd->jpeg)
                RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
        else
                RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

        return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg ) {
        static char     fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
        screenshotCommand_t     *cmd;

        cmd = R_GetCommandBuffer( sizeof( *cmd ) );
        if ( !cmd ) {
                return;
        }
        cmd->commandId = RC_SCREENSHOT;

        cmd->x = x;
        cmd->y = y;
        cmd->width = width;
        cmd->height = height;
        Q_strncpyz( fileName, name, sizeof(fileName) );
        cmd->fileName = fileName;
        cmd->jpeg = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
        int             a,b,c,d;

        if ( lastNumber < 0 || lastNumber > 9999 ) {
                Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
                return;
        }

        a = lastNumber / 1000;
        lastNumber -= a*1000;
        b = lastNumber / 100;
        lastNumber -= b*100;
        c = lastNumber / 10;
        lastNumber -= c*10;
        d = lastNumber;

        Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
                , a, b, c, d );
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
        int             a,b,c,d;

        if ( lastNumber < 0 || lastNumber > 9999 ) {
                Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
                return;
        }

        a = lastNumber / 1000;
        lastNumber -= a*1000;
        b = lastNumber / 100;
        lastNumber -= b*100;
        c = lastNumber / 10;
        lastNumber -= c*10;
        d = lastNumber;

        Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
                , a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
        char            checkname[MAX_OSPATH];
        byte            *buffer;
        byte            *source;
        byte            *src, *dst;
        int                     x, y;
        int                     r, g, b;
        float           xScale, yScale;
        int                     xx, yy;

        sprintf( checkname, "levelshots/%s.tga", tr.world->baseName );

        source = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

        buffer = ri.Hunk_AllocateTempMemory( 128 * 128*3 + 18);
        Com_Memset (buffer, 0, 18);
        buffer[2] = 2;          // uncompressed type
        buffer[12] = 128;
        buffer[14] = 128;
        buffer[16] = 24;        // pixel size

        qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source );

        // resample from source
        xScale = glConfig.vidWidth / 512.0f;
        yScale = glConfig.vidHeight / 384.0f;
        for ( y = 0 ; y < 128 ; y++ ) {
                for ( x = 0 ; x < 128 ; x++ ) {
                        r = g = b = 0;
                        for ( yy = 0 ; yy < 3 ; yy++ ) {
                                for ( xx = 0 ; xx < 4 ; xx++ ) {
                                        src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
                                        r += src[0];
                                        g += src[1];
                                        b += src[2];
                                }
                        }
                        dst = buffer + 18 + 3 * ( y * 128 + x );
                        dst[0] = b / 12;
                        dst[1] = g / 12;
                        dst[2] = r / 12;
                }
        }

        // gamma correct
        if ( glConfig.deviceSupportsGamma ) {
                R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
        }

        ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

        ri.Hunk_FreeTempMemory( buffer );
        ri.Hunk_FreeTempMemory( source );

        ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f (void) {
        char    checkname[MAX_OSPATH];
        static  int     lastNumber = -1;
        qboolean        silent;

        if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
                R_LevelShot();
                return;
        }

        if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
                silent = qtrue;
        } else {
                silent = qfalse;
        }

        if ( ri.Cmd_Argc() == 2 && !silent ) {
                // explicit filename
                Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
        } else {
                // scan for a free filename

                // if we have saved a previous screenshot, don't scan
                // again, because recording demo avis can involve
                // thousands of shots
                if ( lastNumber == -1 ) {
                        lastNumber = 0;
                }
                // scan for a free number
                for ( ; lastNumber <= 9999 ; lastNumber++ ) {
                        R_ScreenshotFilename( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
                }

                if ( lastNumber >= 9999 ) {
                        ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
                        return;
                }

                lastNumber++;
        }

        R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse );

        if ( !silent ) {
                ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
        }
}

void R_ScreenShotJPEG_f (void) {
        char            checkname[MAX_OSPATH];
        static  int     lastNumber = -1;
        qboolean        silent;

        if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
                R_LevelShot();
                return;
        }

        if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
                silent = qtrue;
        } else {
                silent = qfalse;
        }

        if ( ri.Cmd_Argc() == 2 && !silent ) {
                // explicit filename
                Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
        } else {
                // scan for a free filename

                // if we have saved a previous screenshot, don't scan
                // again, because recording demo avis can involve
                // thousands of shots
                if ( lastNumber == -1 ) {
                        lastNumber = 0;
                }
                // scan for a free number
                for ( ; lastNumber <= 9999 ; lastNumber++ ) {
                        R_ScreenshotFilenameJPEG( lastNumber, checkname );

      if (!ri.FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
                }

                if ( lastNumber == 10000 ) {
                        ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
                        return;
                }

                lastNumber++;
        }

        R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue );

        if ( !silent ) {
                ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
        }
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
        const videoFrameCommand_t       *cmd;
        int                                                                                             frameSize;
        int                                                                                             i;

        cmd = (const videoFrameCommand_t *)data;

        qglReadPixels( 0, 0, cmd->width, cmd->height, GL_RGBA,
                        GL_UNSIGNED_BYTE, cmd->captureBuffer );

        // gamma correct
        if( glConfig.deviceSupportsGamma )
                R_GammaCorrect( cmd->captureBuffer, cmd->width * cmd->height * 4 );

        if( cmd->motionJpeg )
        {
                frameSize = SaveJPGToBuffer( cmd->encodeBuffer, clu.cl_aviMotionJpegQuality->integer,
                                cmd->width, cmd->height, cmd->captureBuffer );
                ri.CL_WriteAVIVideoFrame( cmd->encodeBuffer, frameSize );
        }
        else
        {
                frameSize = cmd->width * cmd->height;

                for( i = 0; i < frameSize; i++)    // Pack to 24bpp and swap R and B
                {
                        cmd->encodeBuffer[ i*3 ]     = cmd->captureBuffer[ i*4 + 2 ];
                        cmd->encodeBuffer[ i*3 + 1 ] = cmd->captureBuffer[ i*4 + 1 ];
                        cmd->encodeBuffer[ i*3 + 2 ] = cmd->captureBuffer[ i*4 ];
                }

                ri.CL_WriteAVIVideoFrame( cmd->encodeBuffer, frameSize * 3 );
        }

        return (const void *)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
        qglClearDepth( 1.0f );

        qglCullFace(GL_FRONT);

        qglColor4f (1,1,1,1);

        // initialize downstream texture unit if we're running
        // in a multitexture environment
        if ( qglActiveTextureARB ) {
                GL_SelectTexture( 1 );
                GL_TextureMode( r_textureMode->string );
                GL_TexEnv( GL_MODULATE );
                qglDisable( GL_TEXTURE_2D );
                GL_SelectTexture( 0 );
        }

        GL_SelectTexture(0);
        qglEnable(GL_TEXTURE_2D);
        GL_TextureMode( r_textureMode->string );
        GL_TexEnv( GL_MODULATE );

        qglShadeModel( GL_SMOOTH );
        qglDepthFunc( GL_LEQUAL );

        // the vertex array is always enabled, but the color and texture
        // arrays are enabled and disabled around the compiled vertex array call
        qglEnableClientState (GL_VERTEX_ARRAY);



        //
        // make sure our GL state vector is set correctly
        //
        glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

        qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        qglDepthMask( GL_TRUE );
        qglDisable( GL_DEPTH_TEST );
        qglEnable( GL_SCISSOR_TEST );
        qglDisable( GL_CULL_FACE );
        qglDisable( GL_BLEND );
}


/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void )
{
        cvar_t *sys_cpustring = ri.Cvar_Get( "sys_cpustring", "", 0 );
        const char *enablestrings[] =
        {
                "disabled",
                "enabled"
        };
        const char *fsstrings[] =
        {
                "windowed",
                "fullscreen"
        };

        ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
        ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
        ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
        ri.Printf( PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
        ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
        ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
        ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
        ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
        if ( glConfig.displayFrequency )
        {
                ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
        }
        else
        {
                ri.Printf( PRINT_ALL, "N/A\n" );
        }
        if ( glConfig.deviceSupportsGamma )
        {
                ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
        }
        else
        {
                ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
        }
        ri.Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

        // rendering primitives
        {
                int             primitives;

                // default is to use triangles if compiled vertex arrays are present
                ri.Printf( PRINT_ALL, "rendering primitives: " );
                primitives = r_primitives->integer;
                if ( primitives == 0 ) {
                        if ( qglLockArraysEXT ) {
                                primitives = 2;
                        } else {
                                primitives = 1;
                        }
                }
                if ( primitives == -1 ) {
                        ri.Printf( PRINT_ALL, "none\n" );
                } else if ( primitives == 2 ) {
                        ri.Printf( PRINT_ALL, "single glDrawElements\n" );
                } else if ( primitives == 1 ) {
                        ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
                } else if ( primitives == 3 ) {
                        ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
                }
        }

        ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
        ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
        ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
        ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
        ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
        ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
        ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
        if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 )
        {
                ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
        }
        if ( glConfig.hardwareType == GLHW_RAGEPRO )
        {
                ri.Printf( PRINT_ALL, "HACK: ragePro approximations\n" );
        }
        if ( glConfig.hardwareType == GLHW_RIVA128 )
        {
                ri.Printf( PRINT_ALL, "HACK: riva128 approximations\n" );
        }
        if ( glConfig.smpActive ) {
                ri.Printf( PRINT_ALL, "Using dual processor acceleration\n" );
        }
        if ( r_finish->integer ) {
                ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
        }
}

void GlslInfo_f( void )
{
        int j=0;

        ri.Printf( PRINT_ALL, "------GLSL INFO------\n");

        if ( r_ext_vertex_shader->integer )
        {
                ri.Printf( PRINT_ALL, "GLSL Enabled\n" );
        }
        else
        {
                ri.Printf( PRINT_ALL, "GLSL Disabled\n" );
        }
        ri.Printf( PRINT_ALL, "r_vbo %d\n" , r_ext_vertex_buffer_object->integer);
        ri.Printf( PRINT_ALL, "r_simpleshaders %d\n" , r_simpleshaders->integer);
        ri.Printf( PRINT_ALL, "r_dof %d\n" , r_dof->integer);
        ri.Printf( PRINT_ALL, "r_bloom %d\n" , r_bloom->integer);
        if (r_bloom->integer==-1) {
         ri.Printf( PRINT_ALL, "r_bloomcut %f\n" , r_bloomcut->value);
         ri.Printf( PRINT_ALL, "r_bloomadd %f\n" , r_bloomadd->value);
         ri.Printf( PRINT_ALL, "r_bloomflt %f\n" , r_bloomflt->value);
        }

        for (j=1;j<FS_MAX_SHADERS;j++)
        {
                ri.Printf( PRINT_ALL, "------------\n");
                ri.Printf( PRINT_ALL, "shader %d\n",j);

                ri.Printf( PRINT_ALL, "vs: %d %d %d\n",tr.fragshader[j].vertexShader,tr.fragshader[j].fragShader,tr.fragshader[j].program);
                ri.Printf( PRINT_ALL, "diffuse0: %d\n",tr.fragshader[j].diffuse0);
                ri.Printf( PRINT_ALL, "diffuse1: %d\n",tr.fragshader[j].diffuse1);
                ri.Printf( PRINT_ALL, "bump0: %d\n",tr.fragshader[j].bump0);
                ri.Printf( PRINT_ALL, "bump1: %d\n",tr.fragshader[j].bump1);
                ri.Printf( PRINT_ALL, "normal0: %d\n",tr.fragshader[j].normal0);
                ri.Printf( PRINT_ALL, "normal1: %d\n",tr.fragshader[j].normal1);
                ri.Printf( PRINT_ALL, "cube: %d\n",tr.fragshader[j].cubemap);
                ri.Printf( PRINT_ALL, "lm: %d\n",tr.fragshader[j].lightmap0);
                ri.Printf( PRINT_ALL, "rad0: %d\n",tr.fragshader[j].radbump0);
                ri.Printf( PRINT_ALL, "rad1: %d\n",tr.fragshader[j].radbump1);
                ri.Printf( PRINT_ALL, "rad2: %d\n",tr.fragshader[j].radbump2);
                ri.Printf( PRINT_ALL, "specbump: %d\n",tr.fragshader[j].specbump);
                ri.Printf( PRINT_ALL, "ref: %d\n",tr.fragshader[j].reflectmap);
                ri.Printf( PRINT_ALL, "refract: %d\n",tr.fragshader[j].reflectmap);
                ri.Printf( PRINT_ALL, "lightuv: %d\n",tr.fragshader[j].light_uv_i);
                ri.Printf( PRINT_ALL, "pos: %d\n",tr.fragshader[j].position_i);
                ri.Printf( PRINT_ALL, "rgba: %d\n",tr.fragshader[j].rgba_i);
                ri.Printf( PRINT_ALL, "uvs: %d %d %d\n",tr.fragshader[j].uv_i,tr.fragshader[j].stv_i,tr.fragshader[j].ttv_i);

        }
        ri.Printf( PRINT_ALL, "------END GLSL INFO------\n");

}


/*
===============
R_Register
===============
*/
void R_Register( void )
{
        //
        // latched and archived variables
        //
        r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_ext_multitexture = ri.Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
        ri.Cvar_CheckRange( r_ext_multitexture, 1, 1, qtrue ); //ioq3-urt: forced to 1 for now to avoid a bumpy quirk
        r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
        r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);
        r_ext_vertex_buffer_object = ri.Cvar_Get( "r_ext_vertex_buffer_object", "2", CVAR_ARCHIVE|CVAR_LATCH );
        ri.Cvar_CheckRange( r_ext_vertex_buffer_object, 0, 2, qtrue );
        //ri.Cvar_Alias( r_ext_vertex_buffer_object, "r_vbo");
        r_ext_framebuffer_object = ri.Cvar_Get( "r_ext_framebuffer_object", "1", CVAR_ARCHIVE|CVAR_LATCH );
        r_ext_vertex_shader = ri.Cvar_Get( "r_ext_vertex_shader", "1", CVAR_ARCHIVE|CVAR_LATCH );
        //ri.Cvar_Alias( r_ext_vertex_shader, "r_glsl");

        r_picmip = ri.Cvar_Get ("r_picmip", "1",         CVAR_ARCHIVE | CVAR_LATCH );
        r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic",
                        "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_ext_max_anisotropy = ri.Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );

        r_roundImagesDown = ri.Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_colorMipLevels = ri.Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
        ri.Cvar_CheckRange( r_picmip, 0, 2, qtrue );
        r_detailTextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_stencilbits = ri.Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
        r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_ext_multisample = ri.Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_overBrightBits = ri.Cvar_Get ("r_overBrightBits", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);
        r_mode = ri.Cvar_Get( "r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH );
        r_fullscreen = ri.Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
        r_noborder = Cvar_Get("r_noborder", "0", CVAR_ARCHIVE);
        r_minimize = ri.Cvar_Get( "r_minimize", "0", 0 );
        r_width = ri.Cvar_Get( "r_customwidth", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_height = ri.Cvar_Get( "r_customheight", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_pixelAspect = ri.Cvar_Get( "r_custompixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH );

        r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_vertexLight = ri.Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
        r_uiFullScreen = ri.Cvar_Get( "r_uifullscreen", "0", 0);
        r_subdivisions = ri.Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
        r_smp = ri.Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH);
        r_stereoEnabled = ri.Cvar_Get( "r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
        r_greyscale = ri.Cvar_Get("r_greyscale", "0", CVAR_ARCHIVE | CVAR_LATCH);

        //
        // temporary latched variables that can only change over a restart
        //
        r_fullbright = ri.Cvar_Get ("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT );
        r_mapOverBrightBits = ri.Cvar_Get ("r_mapOverBrightBits", "0", CVAR_LATCH );
        r_intensity = ri.Cvar_Get ("r_intensity", "1", CVAR_LATCH );
        r_singleShader = ri.Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

        //
        // archived variables that can change at any time
        //
        r_lodCurveError = ri.Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
        r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
        r_flares = ri.Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
        r_znear = ri.Cvar_Get( "r_znear", "1", CVAR_CHEAT );
        ri.Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
        r_zproj = ri.Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
        r_stereoSeparation = ri.Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
        r_ignoreGLErrors = ri.Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
        r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
        r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
        r_drawSun = ri.Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
        r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
        r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
        r_finish = ri.Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
        r_textureMode = ri.Cvar_Get( "r_textureMode",
                        "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
        r_swapInterval = ri.Cvar_Get( "r_swapInterval", "0",
                                        CVAR_ARCHIVE | CVAR_LATCH );
        r_gamma = ri.Cvar_Get( "r_gamma", "1.0", CVAR_ARCHIVE );
        r_facePlaneCull = ri.Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

        r_railWidth = ri.Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
        r_railCoreWidth = ri.Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE );
        r_railSegmentLength = ri.Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

        r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

        r_ambientScale = ri.Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
        r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

        r_anaglyphMode = ri.Cvar_Get("r_anaglyphMode", "0", CVAR_ARCHIVE);

        r_minEntityLight = ri.Cvar_Get("r_minEntityLight", "1.5", CVAR_ARCHIVE);
        ri.Cvar_CheckRange( r_minEntityLight, 0.0f, 2.0f, qfalse );

        r_bump = ri.Cvar_Get( "r_bump", "1", CVAR_ARCHIVE | CVAR_LATCH);

        r_postwidth = ri.Cvar_Get( "r_postwidth", "0.15", CVAR_ARCHIVE );
        r_postflt = ri.Cvar_Get( "r_postflt", "-0.3", CVAR_ARCHIVE );
        r_postadd = ri.Cvar_Get( "r_postadd", "0", CVAR_ARCHIVE );

        r_post_gamma       = ri.Cvar_Get( "r_post_gamma", "1.0", CVAR_ARCHIVE );
        r_post_brightness  = ri.Cvar_Get( "r_post_brightness", "0.0", CVAR_ARCHIVE );
        r_post_contrast    = ri.Cvar_Get( "r_post_contrast", "1.0", CVAR_ARCHIVE );
        r_post_lightness   = ri.Cvar_Get( "r_post_lightness", "1.0", CVAR_ARCHIVE );
        r_post_vibrance    = ri.Cvar_Get( "r_post_vibrance", "1.0", CVAR_ARCHIVE );
        r_post_saturation  = ri.Cvar_Get( "r_post_saturation", "1.0", CVAR_ARCHIVE );

        r_bloom = ri.Cvar_Get( "r_bloom", "1", CVAR_ARCHIVE );
        r_bloomcut = ri.Cvar_Get( "r_bloomcut", "0.0", CVAR_ARCHIVE );
        r_bloomadd = ri.Cvar_Get( "r_bloomadd", "0.0", CVAR_ARCHIVE );
        r_bloomflt = ri.Cvar_Get( "r_bloomflt", "0.0", CVAR_ARCHIVE );
        r_bloomfltwidth = ri.Cvar_Get( "r_bloomwidth", "1.0", CVAR_ARCHIVE );
        r_bloomres = ri.Cvar_Get( "r_bloomres", "256", CVAR_ARCHIVE | CVAR_LATCH);
        //ri.Cvar_Alias( r_bloom, "r_glslbloom");

        r_autobump = ri.Cvar_Get( "r_autobump", "1", CVAR_ARCHIVE |CVAR_LATCH );

        r_dof = ri.Cvar_Get( "r_dof", "0", CVAR_ARCHIVE |CVAR_LATCH );
        r_dofscale = ri.Cvar_Get( "r_dofscale", "2", CVAR_ARCHIVE );
        ri.Cvar_CheckRange( r_dofscale, 0.0f, 10.0f, qfalse );
        r_dofblend = ri.Cvar_Get( "r_dofblend", "0.5", CVAR_ARCHIVE ); // 3 frames or so
        ri.Cvar_CheckRange( r_dofblend, 0.001f, 1.0f, qfalse );

        // disables all pixelshader effects, just leaving the r_ext_vertex_buffer_object and FS_CHEAP
        r_simpleshaders = ri.Cvar_Get( "r_simpleshaders", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_glsl_debug = ri.Cvar_Get( "r_glsl_debug", "0", CVAR_ARCHIVE | CVAR_LATCH | CVAR_CHEAT );
        // disable if you cant load sm3 shaders
        r_maxstaticverts = ri.Cvar_Get( "r_maxstaticverts", "1500000", CVAR_ARCHIVE );
        // controls whether or not to use the new triangle-soup decal code
        r_decals = ri.Cvar_Get( "r_decals", "1", CVAR_ARCHIVE | CVAR_LATCH );
        r_fancywater = ri.Cvar_Get( "r_fancywater", "1", CVAR_ARCHIVE | CVAR_LATCH );

        r_fogdensity = ri.Cvar_Get( "r_fogdensity", "0.0", CVAR_ARCHIVE | CVAR_CHEAT );
        r_fog_r = ri.Cvar_Get( "r_fog_r", "0.25", CVAR_ARCHIVE | CVAR_CHEAT );
        r_fog_g = ri.Cvar_Get( "r_fog_g", "0.25", CVAR_ARCHIVE | CVAR_CHEAT );
        r_fog_b = ri.Cvar_Get( "r_fog_b", "0.25", CVAR_ARCHIVE | CVAR_CHEAT );


        //
        // temporary variables that can change at any time
        //
        r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );

        r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
        r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
        r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
        r_saveFontData = ri.Cvar_Get( "r_saveFontData", "0", 0 );

        r_nocurves = ri.Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
        r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
        r_lightmap = ri.Cvar_Get ("r_lightmap", "0", 0 );
        r_portalOnly = ri.Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

        r_flareSize = ri.Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
        r_flareFade = ri.Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);
        r_flareCoeff = ri.Cvar_Get ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

        r_showSmp = ri.Cvar_Get ("r_showSmp", "0", CVAR_CHEAT);
        r_skipBackEnd = ri.Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

        r_measureOverdraw = ri.Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
        r_lodscale = ri.Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
        r_norefresh = ri.Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
        r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
        r_ignore = ri.Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
        r_nocull = ri.Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
        r_novis = ri.Cvar_Get ("r_novis", "0", CVAR_CHEAT);
        r_showcluster = ri.Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
        r_speeds = ri.Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
        r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
        r_logFile = ri.Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
        r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
        r_nobind = ri.Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
        r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
        r_showsky = ri.Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
        r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
        r_clear = ri.Cvar_Get ("r_clear", "0", CVAR_CHEAT);
        r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
        r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
        r_drawBuffer = ri.Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
        r_lockpvs = ri.Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
        r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
        r_shadows = ri.Cvar_Get( "cg_shadows", "1", 0 );
        r_marksOnTriangleMeshes = ri.Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

        r_maxpolys = ri.Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
        r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

        r_GLlibCoolDownMsec = ri.Cvar_Get( "r_GLlibCoolDownMsec", "0", CVAR_ARCHIVE );

        // make sure all the commands added here are also
        // removed in R_Shutdown
        ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
        ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
        ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
        ri.Cmd_AddCommand( "modellist", R_Modellist_f );
        ri.Cmd_AddCommand( "modelist", R_ModeList_f );
        ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
        ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
        ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
        ri.Cmd_AddCommand( "glslinfo", GlslInfo_f );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {
        int     err;
        int i;
        byte *ptr;

        if (!com_quiet->integer)
                ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

        // clear all our internal state
        Com_Memset( &tr, 0, sizeof( tr ) );
        Com_Memset( &backEnd, 0, sizeof( backEnd ) );
        Com_Memset( &tess, 0, sizeof( tess ) );

        if(sizeof(glconfig_t) != 11332)
        {
                ri.Error( ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %zd != 11332", sizeof(glconfig_t));
        }

//      Swap_Init();

        if ( (intptr_t)tess.xyz & 15 ) {
                Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
        }
        Com_Memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

        //
        // init function tables
        //
        for ( i = 0; i < FUNCTABLE_SIZE; i++ )
        {
                tr.sinTable[i]          = sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
                tr.squareTable[i]       = ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
                tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
                tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

                if ( i < FUNCTABLE_SIZE / 2 )
                {
                        if ( i < FUNCTABLE_SIZE / 4 )
                        {
                                tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
                        }
                        else
                        {
                                tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
                        }
                }
                else
                {
                        tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
                }
        }

        R_InitFogTable();

        R_NoiseInit();

        R_Register();

        max_polys = r_maxpolys->integer;
        if (max_polys < MAX_POLYS)
                max_polys = MAX_POLYS;

        max_polyverts = r_maxpolyverts->integer;
        if (max_polyverts < MAX_POLYVERTS)
                max_polyverts = MAX_POLYVERTS;

        ptr = ri.Hunk_Alloc( sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
        backEndData[0] = (backEndData_t *) ptr;
        backEndData[0]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[0] ));
        backEndData[0]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys);
        if ( r_smp->integer ) {
                ptr = ri.Hunk_Alloc( sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
                backEndData[1] = (backEndData_t *) ptr;
                backEndData[1]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[1] ));
                backEndData[1]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys);
        } else {
                backEndData[1] = NULL;
        }
        R_ToggleSmpFrame();

        InitOpenGL();

        R_InitImages();

        R_InitShaders();

        R_InitSkins();

        R_ModelInit();

        R_InitFreeType();


        err = qglGetError();
        if ( err != GL_NO_ERROR )
                ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

        if (!com_quiet->integer)
                ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {

        //
        clu.Com_Totaltimerun_save(); //hackity
        Com_WriteConfiguration(); // to be able to make it into config on /quit. major hax.
        //

        if (!com_quiet->integer)
                ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

        ri.Cmd_RemoveCommand ("modellist");
        ri.Cmd_RemoveCommand ("screenshotJPEG");
        ri.Cmd_RemoveCommand ("screenshot");
        ri.Cmd_RemoveCommand ("imagelist");
        ri.Cmd_RemoveCommand ("shaderlist");
        ri.Cmd_RemoveCommand ("skinlist");
        ri.Cmd_RemoveCommand ("gfxinfo");
        ri.Cmd_RemoveCommand ("glslinfo"); //ioq3-urt
        ri.Cmd_RemoveCommand( "modelist" );
        ri.Cmd_RemoveCommand( "shaderstate" );


        if ( tr.registered ) {
                R_SyncRenderThread();
                R_ShutdownCommandBuffers();
                R_DeleteTextures();
        }

        R_DoneFreeType();

        // shut down platform specific OpenGL stuff
        if ( destroyWindow ) {
                GLimp_Shutdown();
        }

        tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
        R_SyncRenderThread();
        if (!Sys_LowPhysicalMemory()) {
                RB_ShowImages();
        }
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp ) {
        static refexport_t      re;

        ri = *rimp;

        Com_Memset( &re, 0, sizeof( re ) );

        if ( apiVersion != REF_API_VERSION ) {
                ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
                        REF_API_VERSION, apiVersion );
                return NULL;
        }

        // the RE_ functions are Renderer Entry points

        re.Shutdown = RE_Shutdown;

        re.BeginRegistration = RE_BeginRegistration;
        re.RegisterModel = RE_RegisterModel;
        re.RegisterSkin = RE_RegisterSkin;
        re.RegisterShader = RE_RegisterShader;
        re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
        re.LoadWorld = RE_LoadWorldMap;
        re.SetWorldVisData = RE_SetWorldVisData;
        re.EndRegistration = RE_EndRegistration;

        re.BeginFrame = RE_BeginFrame;
        re.EndFrame = RE_EndFrame;

        re.MarkFragments = R_MarkFragments;
        re.LerpTag = R_LerpTag;
        re.ModelBounds = R_ModelBounds;

        re.ClearScene = RE_ClearScene;
        re.AddRefEntityToScene = RE_AddRefEntityToScene;
        re.AddPolyToScene = RE_AddPolyToScene;
        re.LightForPoint = R_LightForPoint;
        re.AddLightToScene = RE_AddLightToScene;
        re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
        re.RenderScene = RE_RenderScene;

        re.SetColor = RE_SetColor;
        re.DrawStretchPic = RE_StretchPic;
        re.DrawStretchRaw = RE_StretchRaw;
        re.UploadCinematic = RE_UploadCinematic;

        re.RegisterFont = RE_RegisterFont;
        re.RemapShader = R_RemapShader;
        re.GetEntityToken = R_GetEntityToken;
        re.inPVS = R_inPVS;

        re.TakeVideoFrame = RE_TakeVideoFrame;

        return &re;
}
