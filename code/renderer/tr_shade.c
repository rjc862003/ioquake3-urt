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
// tr_shade.c

#include "tr_local.h"
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/

/*
================
R_ArrayElementDiscrete

This is just for OpenGL conformance testing, it should never be the fastest
================
*/
static void APIENTRY R_ArrayElementDiscrete( GLint index ) {
        qglColor4ubv( tess.svars.colors[ index ] );
        if ( glState.currenttmu ) {
                qglMultiTexCoord2fARB( 0, tess.svars.texcoords[ 0 ][ index ][0], tess.svars.texcoords[ 0 ][ index ][1] );
                qglMultiTexCoord2fARB( 1, tess.svars.texcoords[ 1 ][ index ][0], tess.svars.texcoords[ 1 ][ index ][1] );
        } else {
                qglTexCoord2fv( tess.svars.texcoords[ 0 ][ index ] );
        }
        qglVertex3fv( tess.xyz[ index ] );
}

/*
===================
R_DrawStripElements

===================
*/
static int              c_vertexes;             // for seeing how long our average strips are
static int              c_begins;
static void R_DrawStripElements( int numIndexes, const glIndex_t *indexes, void ( APIENTRY *element )(GLint) ) {
        int i;
        int last[3] = { -1, -1, -1 };
        qboolean even;

        c_begins++;

        if ( numIndexes <= 0 ) {
                return;
        }

        qglBegin( GL_TRIANGLE_STRIP );

        // prime the strip
        element( indexes[0] );
        element( indexes[1] );
        element( indexes[2] );
        c_vertexes += 3;

        last[0] = indexes[0];
        last[1] = indexes[1];
        last[2] = indexes[2];

        even = qfalse;

        for ( i = 3; i < numIndexes; i += 3 )
        {
                // odd numbered triangle in potential strip
                if ( !even )
                {
                        // check previous triangle to see if we're continuing a strip
                        if ( ( indexes[i+0] == last[2] ) && ( indexes[i+1] == last[1] ) )
                        {
                                element( indexes[i+2] );
                                c_vertexes++;
                                assert( indexes[i+2] < tess.numVertexes );
                                even = qtrue;
                        }
                        // otherwise we're done with this strip so finish it and start
                        // a new one
                        else
                        {
                                qglEnd();

                                qglBegin( GL_TRIANGLE_STRIP );
                                c_begins++;

                                element( indexes[i+0] );
                                element( indexes[i+1] );
                                element( indexes[i+2] );

                                c_vertexes += 3;

                                even = qfalse;
                        }
                }
                else
                {
                        // check previous triangle to see if we're continuing a strip
                        if ( ( last[2] == indexes[i+1] ) && ( last[0] == indexes[i+0] ) )
                        {
                                element( indexes[i+2] );
                                c_vertexes++;

                                even = qfalse;
                        }
                        // otherwise we're done with this strip so finish it and start
                        // a new one
                        else
                        {
                                qglEnd();

                                qglBegin( GL_TRIANGLE_STRIP );
                                c_begins++;

                                element( indexes[i+0] );
                                element( indexes[i+1] );
                                element( indexes[i+2] );
                                c_vertexes += 3;

                                even = qfalse;
                        }
                }

                // cache the last three vertices
                last[0] = indexes[i+0];
                last[1] = indexes[i+1];
                last[2] = indexes[i+2];
        }

        qglEnd();
}



/*
==================
R_DrawElements

Optionally performs our own glDrawElements that looks for strip conditions
instead of using the single glDrawElements call that may be inefficient
without compiled vertex arrays.
==================
*/
static void R_DrawElements( int numIndexes, const glIndex_t *indexes ) {
        int             primitives;

        primitives = r_primitives->integer;

        // default is to use triangles if compiled vertex arrays are present
        if ( primitives == 0 ) {
                if ( qglLockArraysEXT ) {
                        primitives = 2;
                } else {
                        primitives = 1;
                }
        }


        if ( primitives == 2 ) {
                qglDrawElements( GL_TRIANGLES,
                                                numIndexes,
                                                GL_INDEX_TYPE,
                                                indexes );
                return;
        }

        if ( primitives == 1 ) {
                R_DrawStripElements( numIndexes,  indexes, qglArrayElement );
                return;
        }

        if ( primitives == 3 ) {
                R_DrawStripElements( numIndexes,  indexes, R_ArrayElementDiscrete );
                return;
        }

        // anything else will cause no drawing
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t        tess;
static qboolean setArraysOnce;

/*
=================
R_BindAnimatedImage

=================
*/
static void R_BindAnimatedImage( textureBundle_t *bundle ) {
        int             index;

        if ( bundle->isVideoMap ) {
                ri.CIN_RunCinematic(bundle->videoMapHandle);
                ri.CIN_UploadCinematic(bundle->videoMapHandle);
                return;
        }

        if ( bundle->numImageAnimations <= 1 ) {
                GL_Bind( bundle->image[0] );
                return;
        }

        // it is necessary to do this messy calc to make sure animations line up
        // exactly with waveforms of the same frequency
        index = myftol( tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE );
        index >>= FUNCTABLE_SIZE2;

        if ( index < 0 ) {
                index = 0;      // may happen with shader time offsets
        }
        index %= bundle->numImageAnimations;

        GL_Bind( bundle->image[ index ] );
}

/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris (shaderCommands_t *input) {
        GL_Bind( tr.whiteImage );
        qglColor3f (1,1,1);

        GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
        qglDepthRange( 0, 0 );

        qglDisableClientState (GL_COLOR_ARRAY);
        qglDisableClientState (GL_TEXTURE_COORD_ARRAY);

        qglVertexPointer (3, GL_FLOAT, 16, input->xyz); // padded for SIMD

        if (qglLockArraysEXT) {
                qglLockArraysEXT(0, input->numVertexes);
                GLimp_LogComment( "glLockArraysEXT\n" );
        }

        R_DrawElements( input->numIndexes, input->indexes );

        if (qglUnlockArraysEXT) {
                qglUnlockArraysEXT();
                GLimp_LogComment( "glUnlockArraysEXT\n" );
        }
        qglDepthRange( 0, 1 );
}


static void DrawStaticTris (shaderCommands_t *input)
{
        int j;
        int col;
        int uniform_location;
        vec3_t tint;
        srfSurfaceStatic_t *srf;

        RB_SetupFragment(&tr.fragshader[FS_WIREFRAME],NULL);

        GL_SelectTexture(0);
        GL_Bind( tr.whiteImage);

        qglDisableClientState (GL_COLOR_ARRAY);
        qglDisableClientState (GL_TEXTURE_COORD_ARRAY);


        GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
        qglDepthRange( 0, 0 );

        for (j=0;j<tess.numStaticSurfs;j++)
        {
                srf=tess.StaticSurfs[j];

                //multicolor wireframe fun
                col=srf->shader->stages[0]->bundle[0].image[0]->texnum % 6;
                if (col==0) VectorSet(tint,0.5f  ,0.5f ,1); else
                if (col==1) VectorSet(tint,0.5f  ,1    ,0.5f); else
                if (col==2) VectorSet(tint,1     ,0.5f ,0.5f); else
                if (col==3) VectorSet(tint,0.5f ,0.0 ,0); else
                if (col==4) VectorSet(tint,0.5f  ,  0  ,0.5f); else
                /*if (col==5)*/ VectorSet(tint,0     ,0.0, 0.5f);

                uniform_location = qglGetUniformLocationARB(tr.fragshader[FS_WIREFRAME].program, "tint");
                if (uniform_location>-1) qglUniform3fARB(uniform_location,tint[0],tint[1],tint[2]);

                qglDrawElements(GL_TRIANGLES, srf->numIndices , GL_UNSIGNED_INT,  GEOMETRY_INDEX_OFFSET(srf->startIndex)  );
        }

        qglDepthRange( 0, 1 );
}

/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals (shaderCommands_t *input) {
        int             i;
        vec3_t  temp;

        GL_Bind( tr.whiteImage );
        qglColor3f (1,1,1);
        qglDepthRange( 0, 0 );  // never occluded
        GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );

        qglBegin (GL_LINES);
        for (i = 0 ; i < input->numVertexes ; i++) {
                qglVertex3fv (input->xyz[i]);
                VectorMA (input->xyz[i], 2, input->normal[i], temp);
                qglVertex3fv (temp);
        }
        qglEnd ();

        qglDepthRange( 0, 1 );
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

        shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

        tess.numStaticSurfs=0;
        tess.numIndexes = 0;
        tess.numVertexes = 0;
        tess.shader = state;
        tess.fogNum = fogNum;
        tess.dlightBits = 0;            // will be OR'd in by surface functions
        tess.xstages = state->stages;
        tess.numPasses = state->numUnfoggedPasses;
        tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;

        tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
        if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
                tess.shaderTime = tess.shader->clampTime;
        }
}

/*
===================
DrawMultitextured

output = t0 * t1 or t0 + t1

t0 = most upstream according to spec
t1 = most downstream according to spec
===================
*/
static void DrawMultitextured( shaderCommands_t *input, int stage ) {
        shaderStage_t   *pStage;

        pStage = tess.xstages[stage];

        GL_State( pStage->stateBits );

        // this is an ugly hack to work around a GeForce driver
        // bug with multitexture and clip planes
        if ( backEnd.viewParms.isPortal ) {
                qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }

        //
        // base
        //
        GL_SelectTexture( 0 );
        qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
        R_BindAnimatedImage( &pStage->bundle[0] );

        //
        // lightmap/secondary pass
        //
        GL_SelectTexture( 1 );
        qglEnable( GL_TEXTURE_2D );
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

        if ( r_lightmap->integer ) {
                GL_TexEnv( GL_REPLACE );
        } else {
                GL_TexEnv( tess.shader->multitextureEnv );
        }

        qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[1] );

        R_BindAnimatedImage( &pStage->bundle[1] );

        R_DrawElements( input->numIndexes, input->indexes );

        //
        // disable texturing on TEXTURE1, then select TEXTURE0
        //
        //qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
        qglDisable( GL_TEXTURE_2D );

        GL_SelectTexture( 0 );
}



/*
===================
ProjectDlightTexture

Perform dynamic lighting with another rendering pass
===================
*/
#if idppc_altivec
static void ProjectDlightTexture_altivec( void ) {
        int             i, l;
        vec_t   origin0, origin1, origin2;
        float   texCoords0, texCoords1;
        vector float floatColorVec0, floatColorVec1;
        vector float modulateVec, colorVec, zero;
        vector short colorShort;
        vector signed int colorInt;
        vector unsigned char floatColorVecPerm, modulatePerm, colorChar;
        vector unsigned char vSel = VECCONST_UINT8(0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff);
        float   *texCoords;
        byte    *colors;
        byte    clipBits[SHADER_MAX_VERTEXES];
        float   texCoordsArray[SHADER_MAX_VERTEXES][2];
        byte    colorArray[SHADER_MAX_VERTEXES][4];
        unsigned        hitIndexes[SHADER_MAX_INDEXES];
        int             numIndexes;
        float   scale;
        float   radius;
        vec3_t  floatColor;
        float   modulate = 0.0f;

        if ( !backEnd.refdef.num_dlights ) {
                return;
        }

        // There has to be a better way to do this so that floatColor
        // and/or modulate are already 16-byte aligned.
        floatColorVecPerm = vec_lvsl(0,(float *)floatColor);
        modulatePerm = vec_lvsl(0,(float *)&modulate);
        modulatePerm = (vector unsigned char)vec_splat((vector unsigned int)modulatePerm,0);
        zero = (vector float)vec_splat_s8(0);

        for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
                dlight_t        *dl;

                if ( !( tess.dlightBits & ( 1 << l ) ) ) {
                        continue;       // this surface definately doesn't have any of this light
                }
                texCoords = texCoordsArray[0];
                colors = colorArray[0];

                dl = &backEnd.refdef.dlights[l];
                origin0 = dl->transformed[0];
                origin1 = dl->transformed[1];
                origin2 = dl->transformed[2];
                radius = dl->radius;
                scale = 1.0f / radius;

                if(r_greyscale->integer)
                {
                        float luminance;

                        luminance = (dl->color[0] * 255.0f + dl->color[1] * 255.0f + dl->color[2] * 255.0f) / 3;
                        floatColor[0] = floatColor[1] = floatColor[2] = luminance;
                }
                else
                {
                floatColor[0] = dl->color[0] * 255.0f;
                floatColor[1] = dl->color[1] * 255.0f;
                floatColor[2] = dl->color[2] * 255.0f;
                }
                floatColorVec0 = vec_ld(0, floatColor);
                floatColorVec1 = vec_ld(11, floatColor);
                floatColorVec0 = vec_perm(floatColorVec0,floatColorVec0,floatColorVecPerm);
                for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4 ) {
                        int             clip = 0;
                        vec_t dist0, dist1, dist2;

                        dist0 = origin0 - tess.xyz[i][0];
                        dist1 = origin1 - tess.xyz[i][1];
                        dist2 = origin2 - tess.xyz[i][2];

                        backEnd.pc.c_dlightVertexes++;

                        texCoords0 = 0.5f + dist0 * scale;
                        texCoords1 = 0.5f + dist1 * scale;

                        if( !r_dlightBacks->integer &&
                                        // dist . tess.normal[i]
                                        ( dist0 * tess.normal[i][0] +
                                        dist1 * tess.normal[i][1] +
                                        dist2 * tess.normal[i][2] ) < 0.0f ) {
                                clip = 63;
                        } else {
                                if ( texCoords0 < 0.0f ) {
                                        clip |= 1;
                                } else if ( texCoords0 > 1.0f ) {
                                        clip |= 2;
                                }
                                if ( texCoords1 < 0.0f ) {
                                        clip |= 4;
                                } else if ( texCoords1 > 1.0f ) {
                                        clip |= 8;
                                }
                                texCoords[0] = texCoords0;
                                texCoords[1] = texCoords1;

                                // modulate the strength based on the height and color
                                if ( dist2 > radius ) {
                                        clip |= 16;
                                        modulate = 0.0f;
                                } else if ( dist2 < -radius ) {
                                        clip |= 32;
                                        modulate = 0.0f;
                                } else {
                                        dist2 = Q_fabs(dist2);
                                        if ( dist2 < radius * 0.5f ) {
                                                modulate = 1.0f;
                                        } else {
                                                modulate = 2.0f * (radius - dist2) * scale;
                                        }
                                }
                        }
                        clipBits[i] = clip;

                        modulateVec = vec_ld(0,(float *)&modulate);
                        modulateVec = vec_perm(modulateVec,modulateVec,modulatePerm);
                        colorVec = vec_madd(floatColorVec0,modulateVec,zero);
                        colorInt = vec_cts(colorVec,0); // RGBx
                        colorShort = vec_pack(colorInt,colorInt);               // RGBxRGBx
                        colorChar = vec_packsu(colorShort,colorShort);  // RGBxRGBxRGBxRGBx
                        colorChar = vec_sel(colorChar,vSel,vSel);               // RGBARGBARGBARGBA replace alpha with 255
                        vec_ste((vector unsigned int)colorChar,0,(unsigned int *)colors);       // store color
                }

                // build a list of triangles that need light
                numIndexes = 0;
                for ( i = 0 ; i < tess.numIndexes ; i += 3 ) {
                        int             a, b, c;

                        a = tess.indexes[i];
                        b = tess.indexes[i+1];
                        c = tess.indexes[i+2];
                        if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
                                continue;       // not lighted
                        }
                        hitIndexes[numIndexes] = a;
                        hitIndexes[numIndexes+1] = b;
                        hitIndexes[numIndexes+2] = c;
                        numIndexes += 3;
                }

                if ( !numIndexes ) {
                        continue;
                }

                qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
                qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

                qglEnableClientState( GL_COLOR_ARRAY );
                qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

                GL_Bind( tr.dlightImage );
                // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
                // where they aren't rendered
                if ( dl->additive ) {
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
                }
                else {
                        GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
                }
                R_DrawElements( numIndexes, hitIndexes );
                backEnd.pc.c_totalIndexes += numIndexes;
                backEnd.pc.c_dlightIndexes += numIndexes;
        }
}
#endif


static void ProjectDlightTexture_scalar( void ) {
        int             i, l;
        vec3_t  origin;
        float   *texCoords;
        byte    *colors;
        byte    clipBits[SHADER_MAX_VERTEXES];
        float   texCoordsArray[SHADER_MAX_VERTEXES][2];
        byte    colorArray[SHADER_MAX_VERTEXES][4];
        unsigned        hitIndexes[SHADER_MAX_INDEXES];
        int             numIndexes;
        float   scale;
        float   radius;
        vec3_t  floatColor;
        float   modulate = 0.0f;

        if ( !backEnd.refdef.num_dlights ) {
                return;
        }

        for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
                dlight_t        *dl;

                if ( !( tess.dlightBits & ( 1 << l ) ) ) {
                        continue;       // this surface definately doesn't have any of this light
                }
                texCoords = texCoordsArray[0];
                colors = colorArray[0];

                dl = &backEnd.refdef.dlights[l];
                VectorCopy( dl->transformed, origin );
                radius = dl->radius;
                scale = 1.0f / radius;

                if(r_greyscale->integer)
                {
                        float luminance;

                        luminance = (dl->color[0] * 255.0f + dl->color[1] * 255.0f + dl->color[2] * 255.0f) / 3;
                        floatColor[0] = floatColor[1] = floatColor[2] = luminance;
                }
                else
                {
                floatColor[0] = dl->color[0] * 255.0f;
                floatColor[1] = dl->color[1] * 255.0f;
                floatColor[2] = dl->color[2] * 255.0f;
                }

                for ( i = 0 ; i < tess.numVertexes ; i++, texCoords += 2, colors += 4 ) {
                        int             clip = 0;
                        vec3_t  dist;

                        VectorSubtract( origin, tess.xyz[i], dist );

                        backEnd.pc.c_dlightVertexes++;

                        texCoords[0] = 0.5f + dist[0] * scale;
                        texCoords[1] = 0.5f + dist[1] * scale;

                        if( !r_dlightBacks->integer &&
                                        // dist . tess.normal[i]
                                        ( dist[0] * tess.normal[i][0] +
                                        dist[1] * tess.normal[i][1] +
                                        dist[2] * tess.normal[i][2] ) < 0.0f ) {
                                clip = 63;
                        } else {
                                if ( texCoords[0] < 0.0f ) {
                                        clip |= 1;
                                } else if ( texCoords[0] > 1.0f ) {
                                        clip |= 2;
                                }
                                if ( texCoords[1] < 0.0f ) {
                                        clip |= 4;
                                } else if ( texCoords[1] > 1.0f ) {
                                        clip |= 8;
                                }
                                texCoords[0] = texCoords[0];
                                texCoords[1] = texCoords[1];

                                // modulate the strength based on the height and color
                                if ( dist[2] > radius ) {
                                        clip |= 16;
                                        modulate = 0.0f;
                                } else if ( dist[2] < -radius ) {
                                        clip |= 32;
                                        modulate = 0.0f;
                                } else {
                                        dist[2] = Q_fabs(dist[2]);
                                        if ( dist[2] < radius * 0.5f ) {
                                                modulate = 1.0f;
                                        } else {
                                                modulate = 2.0f * (radius - dist[2]) * scale;
                                        }
                                }
                        }
                        clipBits[i] = clip;
                        colors[0] = myftol(floatColor[0] * modulate);
                        colors[1] = myftol(floatColor[1] * modulate);
                        colors[2] = myftol(floatColor[2] * modulate);
                        colors[3] = 255;
                }

                // build a list of triangles that need light
                numIndexes = 0;
                for ( i = 0 ; i < tess.numIndexes ; i += 3 ) {
                        int             a, b, c;

                        a = tess.indexes[i];
                        b = tess.indexes[i+1];
                        c = tess.indexes[i+2];
                        if ( clipBits[a] & clipBits[b] & clipBits[c] ) {
                                continue;       // not lighted
                        }
                        hitIndexes[numIndexes] = a;
                        hitIndexes[numIndexes+1] = b;
                        hitIndexes[numIndexes+2] = c;
                        numIndexes += 3;
                }

                if ( !numIndexes ) {
                        continue;
                }

                qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
                qglTexCoordPointer( 2, GL_FLOAT, 0, texCoordsArray[0] );

                qglEnableClientState( GL_COLOR_ARRAY );
                qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, colorArray );

                GL_Bind( tr.dlightImage );
                // include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
                // where they aren't rendered
                if ( dl->additive ) {
                        GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
                }
                else {
                        GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
                }
                R_DrawElements( numIndexes, hitIndexes );
                backEnd.pc.c_totalIndexes += numIndexes;
                backEnd.pc.c_dlightIndexes += numIndexes;
        }
}

static void ProjectDlightTexture( void ) {
#if idppc_altivec
        if (com_altivec->integer) {
                // must be in a seperate function or G3 systems will crash.
                ProjectDlightTexture_altivec();
                return;
        }
#endif
        //r00tDebug
//        if (backEnd.refdef.num_dlights) fprintf(stderr,"ProjectDlightTexture(%d) verts %d\n",backEnd.refdef.num_dlights,tess.numVertexes);
        // TODO: VBOs don't support Dlights yet... it crashes
//        if (!r_ext_vertex_buffer_object->integer) //@r00t: Not really, seems to work
        ProjectDlightTexture_scalar();
}


/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
        fog_t           *fog;
        int                     i;

        qglEnableClientState( GL_COLOR_ARRAY );
        qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

        qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
        qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );

        fog = tr.world->fogs + tess.fogNum;

        for ( i = 0; i < tess.numVertexes; i++ ) {
                * ( int * )&tess.svars.colors[i] = fog->colorInt;
        }

        RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[0] );

        GL_Bind( tr.fogImage );

        if ( tess.shader->fogPass == FP_EQUAL ) {
                GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
        } else {
                GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
        }

        R_DrawElements( tess.numIndexes, tess.indexes );
}

/*
===============
ComputeColors
===============
*/
static void ComputeColors( shaderStage_t *pStage )
{
        int             i;

        //
        // rgbGen
        //
        switch ( pStage->rgbGen )
        {
                case CGEN_IDENTITY:
                        Com_Memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
                        break;
                default:
                case CGEN_IDENTITY_LIGHTING:
                        Com_Memset( tess.svars.colors, tr.identityLightByte, tess.numVertexes * 4 );
                        break;
                case CGEN_LIGHTING_DIFFUSE:
                        RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );
                        break;
                case CGEN_EXACT_VERTEX:
                        Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
                        break;
                case CGEN_CONST:
                        for ( i = 0; i < tess.numVertexes; i++ ) {
                                *(int *)tess.svars.colors[i] = *(int *)pStage->constantColor;
                        }
                        break;
                case CGEN_VERTEX:
                        if ( tr.identityLight == 1 )
                        {
                                Com_Memcpy( tess.svars.colors, tess.vertexColors, tess.numVertexes * sizeof( tess.vertexColors[0] ) );
                        }
                        else
                        {
                                for ( i = 0; i < tess.numVertexes; i++ )
                                {
                                        tess.svars.colors[i][0] = tess.vertexColors[i][0] * tr.identityLight;
                                        tess.svars.colors[i][1] = tess.vertexColors[i][1] * tr.identityLight;
                                        tess.svars.colors[i][2] = tess.vertexColors[i][2] * tr.identityLight;
                                        tess.svars.colors[i][3] = tess.vertexColors[i][3];
                                }
                        }
                        break;
                case CGEN_ONE_MINUS_VERTEX:
                        if ( tr.identityLight == 1 )
                        {
                                for ( i = 0; i < tess.numVertexes; i++ )
                                {
                                        tess.svars.colors[i][0] = 255 - tess.vertexColors[i][0];
                                        tess.svars.colors[i][1] = 255 - tess.vertexColors[i][1];
                                        tess.svars.colors[i][2] = 255 - tess.vertexColors[i][2];
                                }
                        }
                        else
                        {
                                for ( i = 0; i < tess.numVertexes; i++ )
                                {
                                        tess.svars.colors[i][0] = ( 255 - tess.vertexColors[i][0] ) * tr.identityLight;
                                        tess.svars.colors[i][1] = ( 255 - tess.vertexColors[i][1] ) * tr.identityLight;
                                        tess.svars.colors[i][2] = ( 255 - tess.vertexColors[i][2] ) * tr.identityLight;
                                }
                        }
                        break;
                case CGEN_FOG:
                        {
                                fog_t           *fog;

                                fog = tr.world->fogs + tess.fogNum;

                                for ( i = 0; i < tess.numVertexes; i++ ) {
                                        * ( int * )&tess.svars.colors[i] = fog->colorInt;
                                }
                        }
                        break;
                case CGEN_WAVEFORM:
                        RB_CalcWaveColor( &pStage->rgbWave, ( unsigned char * ) tess.svars.colors );
                        break;
                case CGEN_ENTITY:
                        RB_CalcColorFromEntity( ( unsigned char * ) tess.svars.colors );
                        break;
                case CGEN_ONE_MINUS_ENTITY:
                        RB_CalcColorFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
                        break;
        }

        //
        // alphaGen
        //
        switch ( pStage->alphaGen )
        {
        case AGEN_SKIP:
                break;
        case AGEN_IDENTITY:
                if ( pStage->rgbGen != CGEN_IDENTITY ) {
                        if ( ( pStage->rgbGen == CGEN_VERTEX && tr.identityLight != 1 ) ||
                                 pStage->rgbGen != CGEN_VERTEX ) {
                                for ( i = 0; i < tess.numVertexes; i++ ) {
                                        tess.svars.colors[i][3] = 0xff;
                                }
                        }
                }
                break;
        case AGEN_CONST:
                if ( pStage->rgbGen != CGEN_CONST ) {
                        for ( i = 0; i < tess.numVertexes; i++ ) {
                                tess.svars.colors[i][3] = pStage->constantColor[3];
                        }
                }
                break;
        case AGEN_WAVEFORM:
                RB_CalcWaveAlpha( &pStage->alphaWave, ( unsigned char * ) tess.svars.colors );
                break;
        case AGEN_LIGHTING_SPECULAR:
                RB_CalcSpecularAlpha( ( unsigned char * ) tess.svars.colors );
                break;
        case AGEN_ENTITY:
                RB_CalcAlphaFromEntity( ( unsigned char * ) tess.svars.colors );
                break;
        case AGEN_ONE_MINUS_ENTITY:
                RB_CalcAlphaFromOneMinusEntity( ( unsigned char * ) tess.svars.colors );
                break;
    case AGEN_VERTEX:
                if ( pStage->rgbGen != CGEN_VERTEX ) {
                        for ( i = 0; i < tess.numVertexes; i++ ) {
                                tess.svars.colors[i][3] = tess.vertexColors[i][3];
                        }
                }
        break;
    case AGEN_ONE_MINUS_VERTEX:
        for ( i = 0; i < tess.numVertexes; i++ )
        {
                        tess.svars.colors[i][3] = 255 - tess.vertexColors[i][3];
        }
        break;
        case AGEN_PORTAL:
                {
                        unsigned char alpha;

                        for ( i = 0; i < tess.numVertexes; i++ )
                        {
                                float len;
                                vec3_t v;

                                VectorSubtract( tess.xyz[i], backEnd.viewParms.or.origin, v );
                                len = VectorLength( v );

                                len /= tess.shader->portalRange;

                                if ( len < 0 )
                                {
                                        alpha = 0;
                                }
                                else if ( len > 1 )
                                {
                                        alpha = 0xff;
                                }
                                else
                                {
                                        alpha = len * 0xff;
                                }

                                tess.svars.colors[i][3] = alpha;
                        }
                }
                break;
        }

        //
        // fog adjustment for colors to fade out as fog increases
        //
        if ( tess.fogNum )
        {
                switch ( pStage->adjustColorsForFog )
                {
                case ACFF_MODULATE_RGB:
                        RB_CalcModulateColorsByFog( ( unsigned char * ) tess.svars.colors );
                        break;
                case ACFF_MODULATE_ALPHA:
                        RB_CalcModulateAlphasByFog( ( unsigned char * ) tess.svars.colors );
                        break;
                case ACFF_MODULATE_RGBA:
                        RB_CalcModulateRGBAsByFog( ( unsigned char * ) tess.svars.colors );
                        break;
                case ACFF_NONE:
                        break;
                }
        }

        // if in greyscale rendering mode turn all color values into greyscale.
        if(r_greyscale->integer)
        {
                int scale;

                for(i = 0; i < tess.numVertexes; i++)
                {
                        scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2]) / 3;
                        tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
                }
        }
}

/*
===============
ComputeTexCoords
===============
*/
static void ComputeTexCoords( shaderStage_t *pStage ) {
        int             i;
        int             b;

        for ( b = 0; b < NUM_TEXTURE_BUNDLES; b++ ) {
                int tm;

                //
                // generate the texture coordinates
                //
                switch ( pStage->bundle[b].tcGen )
                {
                case TCGEN_IDENTITY:
                        Com_Memset( tess.svars.texcoords[b], 0, sizeof( float ) * 2 * tess.numVertexes );
                        break;
                case TCGEN_TEXTURE:
                        for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                                tess.svars.texcoords[b][i][0] = tess.texCoords[i][0][0];
                                tess.svars.texcoords[b][i][1] = tess.texCoords[i][0][1];
                        }
                        break;
                case TCGEN_LIGHTMAP:
                        for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                                tess.svars.texcoords[b][i][0] = tess.texCoords[i][1][0];
                                tess.svars.texcoords[b][i][1] = tess.texCoords[i][1][1];
                        }
                        break;
                case TCGEN_VECTOR:
                        for ( i = 0 ; i < tess.numVertexes ; i++ ) {
                                tess.svars.texcoords[b][i][0] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[0] );
                                tess.svars.texcoords[b][i][1] = DotProduct( tess.xyz[i], pStage->bundle[b].tcGenVectors[1] );
                        }
                        break;
                case TCGEN_FOG:
                        RB_CalcFogTexCoords( ( float * ) tess.svars.texcoords[b] );
                        break;
                case TCGEN_ENVIRONMENT_MAPPED:
                        RB_CalcEnvironmentTexCoords( ( float * ) tess.svars.texcoords[b] );
                        break;
                case TCGEN_BAD:
                        return;
                }

                //
                // alter texture coordinates
                //
                for ( tm = 0; tm < pStage->bundle[b].numTexMods ; tm++ ) {
                        switch ( pStage->bundle[b].texMods[tm].type )
                        {
                        case TMOD_NONE:
                                tm = TR_MAX_TEXMODS;            // break out of for loop
                                break;

                        case TMOD_TURBULENT:
                                RB_CalcTurbulentTexCoords( &pStage->bundle[b].texMods[tm].wave,
                                                                 ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_ENTITY_TRANSLATE:
                                RB_CalcScrollTexCoords( backEnd.currentEntity->e.shaderTexCoord,
                                                                         ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_SCROLL:
                                RB_CalcScrollTexCoords( pStage->bundle[b].texMods[tm].scroll,
                                                                                 ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_SCALE:
                                RB_CalcScaleTexCoords( pStage->bundle[b].texMods[tm].scale,
                                                                         ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_STRETCH:
                                RB_CalcStretchTexCoords( &pStage->bundle[b].texMods[tm].wave,
                                                               ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_TRANSFORM:
                                RB_CalcTransformTexCoords( &pStage->bundle[b].texMods[tm],
                                                                 ( float * ) tess.svars.texcoords[b] );
                                break;

                        case TMOD_ROTATE:
                                RB_CalcRotateTexCoords( pStage->bundle[b].texMods[tm].rotateSpeed,
                                                                                ( float * ) tess.svars.texcoords[b] );
                                break;

                        default:
                                ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", pStage->bundle[b].texMods[tm].type, tess.shader->name );
                                break;
                        }
                }
        }
}

/*
** RB_IterateStagesGeneric
*/
static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
        int stage;

        for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
        {
                shaderStage_t *pStage = tess.xstages[stage];

                if ( !pStage )
                {
                        break;
                }

                ComputeColors( pStage );
                ComputeTexCoords( pStage );

                if ( !setArraysOnce )
                {
                        qglEnableClientState( GL_COLOR_ARRAY );
                        qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, input->svars.colors );
                }

                //
                // do multitexture
                //
                if ( pStage->bundle[1].image[0] != 0 )
                {
                        DrawMultitextured( input, stage );
                }
                else
                {
                        if ( !setArraysOnce )
                        {
                                qglTexCoordPointer( 2, GL_FLOAT, 0, input->svars.texcoords[0] );
                        }

                        //
                        // set state
                        //
                        if ( pStage->bundle[0].vertexLightmap && ( (r_vertexLight->integer && !r_uiFullScreen->integer) || glConfig.hardwareType == GLHW_PERMEDIA2 ) && r_lightmap->integer )
                        {
                                GL_Bind( tr.whiteImage );
                        }
                        else
                                R_BindAnimatedImage( &pStage->bundle[0] );

                        GL_State( pStage->stateBits );

                        //
                        // draw
                        //
                        R_DrawElements( input->numIndexes, input->indexes );
                }
                // allow skipping out to show just lightmaps during development
                if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
                {
                        break;
                }
        }
}


/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
        shaderCommands_t *input;

        input = &tess;
        RB_DeformTessGeometry();
        //
        // log this call
        //
        if ( r_logFile->integer )
        {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );

        // set polygon offset if necessary
        if ( input->shader->polygonOffset )
        {
                qglEnable( GL_POLYGON_OFFSET_FILL );
                qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
        }

        //
        // if there is only a single pass then we can enable color
        // and texture arrays before we compile, otherwise we need
        // to avoid compiling those arrays since they will change
        // during multipass rendering
        //
        if ( tess.numPasses > 1 || input->shader->multitextureEnv )
        {
                setArraysOnce = qfalse;
                qglDisableClientState (GL_COLOR_ARRAY);
                qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
        }
        else
        {
                setArraysOnce = qtrue;

                qglEnableClientState( GL_COLOR_ARRAY);
                qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );

                qglEnableClientState( GL_TEXTURE_COORD_ARRAY);
                qglTexCoordPointer( 2, GL_FLOAT, 0, tess.svars.texcoords[0] );
        }

        //
        // lock XYZ
        //
        qglVertexPointer (3, GL_FLOAT, 16, input->xyz); // padded for SIMD
        if (qglLockArraysEXT)
        {
                qglLockArraysEXT(0, input->numVertexes);
                GLimp_LogComment( "glLockArraysEXT\n" );
        }

        //
        // enable color and texcoord arrays after the lock if necessary
        //
        if ( !setArraysOnce )
        {
                qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
                qglEnableClientState( GL_COLOR_ARRAY );
        }

        //
        // call shader function
        //
        RB_IterateStagesGeneric( input );

        //
        // now do any dynamic lighting needed
        //
        if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
                && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
                ProjectDlightTexture();
        }

        //
        // now do fog
        //
        if ( tess.fogNum && tess.shader->fogPass ) {
                RB_FogPass();
        }

        //
        // unlock arrays
        //
        if (qglUnlockArraysEXT)
        {
                qglUnlockArraysEXT();
                GLimp_LogComment( "glUnlockArraysEXT\n" );
        }

        //
        // reset polygon offset
        //
        if ( input->shader->polygonOffset )
        {
                qglDisable( GL_POLYGON_OFFSET_FILL );
        }

}


/*
** RB_StageIteratorVertexLitTexture
*/
void RB_StageIteratorVertexLitTexture( void )
{
        shaderCommands_t *input;
        shader_t                *shader;

        input = &tess;

        shader = input->shader;

        //
        // compute colors
        //
        RB_CalcDiffuseColor( ( unsigned char * ) tess.svars.colors );

        //
        // log this call
        //
        if ( r_logFile->integer )
        {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorVertexLitTexturedUnfogged( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );

        //
        // set arrays and lock
        //
        qglEnableClientState( GL_COLOR_ARRAY);
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY);

        qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.svars.colors );
        qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );
        qglVertexPointer (3, GL_FLOAT, 16, input->xyz);

        if ( qglLockArraysEXT )
        {
                qglLockArraysEXT(0, input->numVertexes);
                GLimp_LogComment( "glLockArraysEXT\n" );
        }

        //
        // call special shade routine
        //
        R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
        GL_State( tess.xstages[0]->stateBits );
        R_DrawElements( input->numIndexes, input->indexes );

        //
        // now do any dynamic lighting needed
        //
        if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
                ProjectDlightTexture();
        }

        //
        // now do fog
        //
        if ( tess.fogNum && tess.shader->fogPass ) {
                RB_FogPass();
        }

        //
        // unlock arrays
        //
        if (qglUnlockArraysEXT)
        {
                qglUnlockArraysEXT();
                GLimp_LogComment( "glUnlockArraysEXT\n" );
        }
}

//define        REPLACE_MODE

void RB_StageIteratorLightmappedMultitexture( void )
{
        shaderCommands_t *input;

        input = &tess;

        //
        // log this call
        //
        if ( r_logFile->integer ) {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorLightmappedMultitexture( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );

        //
        // set color, pointers, and lock
        //
        GL_State( GLS_DEFAULT );
        qglVertexPointer( 3, GL_FLOAT, 16, input->xyz );

#ifdef REPLACE_MODE
        qglDisableClientState( GL_COLOR_ARRAY );
        qglColor3f( 1, 1, 1 );
        qglShadeModel( GL_FLAT );
#else
        qglEnableClientState( GL_COLOR_ARRAY );
        qglColorPointer( 4, GL_UNSIGNED_BYTE, 0, tess.constantColor255 );
#endif

        //
        // select base stage
        //
        GL_SelectTexture( 0 );

        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        R_BindAnimatedImage( &tess.xstages[0]->bundle[0] );
        qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][0] );

        //
        // configure second stage
        //
        GL_SelectTexture( 1 );
        qglEnable( GL_TEXTURE_2D );
        if ( r_lightmap->integer ) {
                GL_TexEnv( GL_REPLACE );
        } else {
                GL_TexEnv( GL_MODULATE );
        }
        R_BindAnimatedImage( &tess.xstages[0]->bundle[1] );
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 16, tess.texCoords[0][1] );

        //
        // lock arrays
        //
        if ( qglLockArraysEXT ) {
                qglLockArraysEXT(0, input->numVertexes);
                GLimp_LogComment( "glLockArraysEXT\n" );
        }

        R_DrawElements( input->numIndexes, input->indexes );

        //
        // disable texturing on TEXTURE1, then select TEXTURE0
        //
        qglDisable( GL_TEXTURE_2D );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );

        GL_SelectTexture( 0 );
#ifdef REPLACE_MODE
        GL_TexEnv( GL_MODULATE );
        qglShadeModel( GL_SMOOTH );
#endif

        //
        // now do any dynamic lighting needed
        //
        if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE ) {
                ProjectDlightTexture();
        }

        //
        // now do fog
        //
        if ( tess.fogNum && tess.shader->fogPass ) {
                RB_FogPass();
        }

        //
        // unlock arrays
        //
        if ( qglUnlockArraysEXT ) {
                qglUnlockArraysEXT();
                GLimp_LogComment( "glUnlockArraysEXT\n" );
        }

}



void RB_StageIteratorCheapFancyWater_GLSL( void )
{
        shaderCommands_t *input;
        srfSurfaceStatic_t *srf;
        int j;
        int diffuse,normal,bump,cube;

        input = &tess;

        //call the program
        RB_SetupFragment(tess.shader->stages[0]->program,tess.shader);

        //
        // log this call
        //
        if ( r_logFile->integer ) {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorCheapFancyWater_GLSL( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );


        GL_State( GLS_DEFAULT );

        GL_State( input->shader->stages[0]->stateBits);
        //Todo: get vertex colors encoded
        //              although I cant recall any maps actually using these still

        qglColor3f (1,1,1);

        //Diffuse
        diffuse=tess.xstages[0]->bundle[0].image[0]->texnum;
        if (tess.xstages[0]->normalmap!=NULL)
        {
                normal= tess.xstages[0]->normalmap->texnum;
        }
        else
        {
                normal=tr.normalImage->texnum;
        }
        //bump/offset map
        if (tess.xstages[0]->bumpmap!=NULL)
        {
                bump=  tess.xstages[0]->bumpmap->texnum;
        }
        else
        {
                bump=tr.greyImage->texnum;
        }

        for (j=0;j<tess.numStaticSurfs;j++)
        {
                srf=tess.StaticSurfs[j];
                //cubemap

                if (tr.refdef.rdflags & RDF_NOCUBEMAP ||  !srf->cubemap || normal==tr.normalImage->texnum)
                {
                        cube= tr.blackCubeImage->texnum;
                }
                else
                {
                        cube= srf->cubemap->texnum;
                }
                RB_BindFragmentTextures(glState.currentFragShader,diffuse,normal,bump ,0,0,0,cube,0);

                qglDrawElements(GL_TRIANGLES, srf->numIndices, GL_UNSIGNED_INT, GEOMETRY_INDEX_OFFSET(srf->startIndex));
                }
        }

void RB_SetVertexRGB_GLSL(      shaderStage_t   *pStage)
{
        vec3_t forceColor={1,1,1};
        float forceAlpha=0;
        float RGBMode=0;
        float AlphaMode=0;
        int uniform_location;
        //Bind the correct vertex colors
        switch (pStage->rgbGen)
        {
                case CGEN_IDENTITY:
                        //Com_Memset( tess.svars.colors, 0xff, tess.numVertexes * 4 );
                        VectorSet(forceColor,1,1,1);
                        RGBMode=0;
                        break;
                default:
                case CGEN_IDENTITY_LIGHTING:

                        VectorSet(forceColor,tr.identityLight,tr.identityLight,tr.identityLight);
                        RGBMode=0;
                        break;
                case CGEN_LIGHTING_DIFFUSE:

                        RGBMode=1;
                        break;
                case CGEN_EXACT_VERTEX:

                        RGBMode=1;
                        break;
                case CGEN_CONST:

                        VectorSet(forceColor,pStage->constantColor[0]/255.0f,pStage->constantColor[1]/255.0f,pStage->constantColor[2]/255.0f);
                        RGBMode=0;
                        break;
                case CGEN_VERTEX:
                        RGBMode=1;

                        break;
                case CGEN_ONE_MINUS_VERTEX:
                        RGBMode=2;
                        break;
                case CGEN_WAVEFORM:
                        RGBMode=1; //wrong/todo
                        break;

        }


        uniform_location = qglGetUniformLocationARB(pStage->program->program, "rgbmode");
        if (uniform_location>-1) qglUniform1fARB(uniform_location,RGBMode);
        uniform_location = qglGetUniformLocationARB(pStage->program->program, "alphamode");
        if (uniform_location>-1) qglUniform1fARB(uniform_location,AlphaMode);
        uniform_location = qglGetUniformLocationARB(pStage->program->program, "rgbcolor");
        if (uniform_location>-1) qglUniform4fARB(uniform_location,forceColor[0],forceColor[1],forceColor[2],forceAlpha);

}

void RB_StageIteratorLightmappedMultitexture_GLSL( void )
{
        shaderCommands_t *input;
        srfSurfaceStatic_t *srf;
        int j;
        int diffuse,normal,bump,cube;
        shaderStage_t   *pStage;
        int             i;
        deformStage_t   *ds;
        int uniform_location;
        fragShader_t* frag;


        input = &tess;

        frag=tess.shader->stages[0]->program;
        pStage=tess.shader->stages[0];
        //call the program
        RB_SetupFragment(pStage->program,tess.shader);


        if (strstr(tess.shader->name,"textures/lundesert/grass1"))
        {
                input = &tess;
        }
        //
        // log this call
        //
        if ( r_logFile->integer ) {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorLightmappedMultitexture_GLSL( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        GL_Cull( input->shader->cullType );

        // set polygon offset if necessary
        if ( input->shader->polygonOffset )
        {
                qglEnable( GL_POLYGON_OFFSET_FILL );
                qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
        }



        GL_State( GLS_DEFAULT );

        GL_State(  pStage->stateBits);

        qglColor3f (1,1,1);

        //Diffuse
        diffuse=tess.xstages[0]->bundle[0].image[0]->texnum;
        if (tess.xstages[0]->normalmap!=NULL)
        {
                normal= tess.xstages[0]->normalmap->texnum;
        }
        else
        {
                normal=tr.normalImage->texnum;
        }
        //bump/offset map
        if (tess.xstages[0]->bumpmap!=NULL)
        {
                bump=  tess.xstages[0]->bumpmap->texnum;
        }
        else
        {
                bump=tr.greyImage->texnum;
        }

        RB_SetVertexRGB_GLSL(pStage);



        for ( i = 0 ; i < tess.shader->numDeforms ; i++ )
        {
                ds = &tess.shader->deforms[ i ];

                switch ( ds->deformation )
                {
                        case DEFORM_WAVE:
                        //RB_CalcDeformVertexes( ds );
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_deformtype");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationWave.func );
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_deformationSpread");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationSpread);
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_phase");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationWave.phase);
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_frequency");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationWave.frequency );
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_amplitude");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationWave.amplitude );
                                uniform_location = qglGetUniformLocationARB(frag->program, "IN_base");
                                if (uniform_location>-1) qglUniform1fARB(uniform_location, ds->deformationWave.base );

                        break;
                        case DEFORM_BULGE:
                //              RB_CalcBulgeVertexes( ds );
                        break;
                        case DEFORM_MOVE:
                        //      RB_CalcMoveVertexes( ds );
                        break;
                        default:
                        break;
                }
        }

        for (j=0;j<tess.numStaticSurfs;j++)
        {
                srf=tess.StaticSurfs[j];
                //cubemap

                if (tr.refdef.rdflags & RDF_NOCUBEMAP ||  !srf->cubemap || normal==tr.normalImage->texnum)
                {
                        cube= tr.blackCubeImage->texnum;
                }
                else
                {
                        cube= srf->cubemap->texnum;
                }
                RB_BindFragmentTextures(glState.currentFragShader,diffuse,normal,bump ,0,0,0,cube,0);

                qglDrawElements(GL_TRIANGLES, srf->numIndices , GL_UNSIGNED_INT,  GEOMETRY_INDEX_OFFSET(srf->startIndex)  );
        }


        if ( input->shader->polygonOffset )
        {
                qglDisable( GL_POLYGON_OFFSET_FILL );
        }
        uniform_location = qglGetUniformLocationARB(frag->program, "IN_deformtype");
        if (uniform_location>-1) qglUniform1fARB(uniform_location, 0);

        // r00tTODO
        // now do any dynamic lighting needed
        //
        if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
                && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {

//TODO: Copy verts from VBO to tess so Dlights works... this may impact FPS badly
/*
        float *ptr = (float*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_READ_WRITE_ARB);
        if(ptr)
        {
            //    ProjectDlightTexture();

            glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); // release pointer to mapping buffer
        }
*/

        }

}


void RB_StageIteratorLightmappedTerrain_GLSL( void )
{
        shaderCommands_t *input;
        srfSurfaceStatic_t *srf;
        int j;
        int diffuse,normal,bump,cube;
        int diffuse1,normal1,bump1;

        input = &tess;

        //call the program
        RB_SetupFragment(tess.shader->stages[0]->program,tess.shader);

        //
        // log this call
        //
        if ( r_logFile->integer ) {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorLightmappedMultitexture_GLSL( %s ) ---\n", tess.shader->name) );
        }

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );

        // set polygon offset if necessary
        if ( input->shader->polygonOffset )
        {
                qglEnable( GL_POLYGON_OFFSET_FILL );
                qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
        }



        GL_State( GLS_DEFAULT );

        //Todo: get vertex colors encoded
        //              although I cant recall any maps actually using these still

        qglColor3f (1,1,1);

        //Diffuse
        diffuse=tess.xstages[0]->bundle[0].image[0]->texnum;
        if (tess.xstages[0]->normalmap!=NULL)
        {
                normal= tess.xstages[0]->normalmap->texnum;
        }
        else
        {
                normal=tr.normalImage->texnum;
        }
        //bump/offset map
        if (tess.xstages[0]->bumpmap!=NULL)
        {
                bump=  tess.xstages[0]->bumpmap->texnum;
        }
        else
        {
                bump=tr.greyImage->texnum;
        }

        //Diffuse
        diffuse1=tess.xstages[1]->bundle[0].image[0]->texnum;
        if (tess.xstages[1]->normalmap!=NULL)
        {
                normal1= tess.xstages[1]->normalmap->texnum;
        }
        else
        {
                normal1=tr.normalImage->texnum;
        }
        //bump/offset map
        if (tess.xstages[1]->bumpmap!=NULL)
        {
                bump1=  tess.xstages[1]->bumpmap->texnum;
        }
        else
        {
                bump1=tr.greyImage->texnum;
        }

        for (j=0;j<tess.numStaticSurfs;j++)
        {
                srf=tess.StaticSurfs[j];
                //cubemap

                if (tr.refdef.rdflags & RDF_NOCUBEMAP ||  !srf->cubemap || normal==tr.normalImage->texnum)
                {
                        cube= tr.blackCubeImage->texnum;
                }
                else
                {
                        cube= srf->cubemap->texnum;
                }
                RB_BindFragmentTextures(glState.currentFragShader,diffuse,normal,bump ,diffuse1,normal1,bump1,cube,0);



                qglDrawElements(GL_TRIANGLES, srf->numIndices , GL_UNSIGNED_INT,  GEOMETRY_INDEX_OFFSET(srf->startIndex)  );
        }




}




void RB_StageIteratorWater_GLSL( void ) {
        shaderCommands_t *input;
        srfSurfaceStatic_t *srf;
        int j;

        input = &tess;

        //return;
        //
        // log this call
        //
        if ( r_logFile->integer ) {
                // don't just call LogComment, or we will get
                // a call to va() every frame!
                GLimp_LogComment( va("--- RB_StageIteratorWater( %s ) ---\n", tess.shader->name) );
        }

        if (!(backEnd.viewParms.doWater && backEnd.viewParms.isWater==0)) return;

        //
        // set face culling appropriately
        //
        GL_Cull( input->shader->cullType );

        GL_State( GLS_DEFAULT );
        RB_SetupFragment(&tr.fragshader[FS_WATER],tess.shader);

        qglColor3f (1,1,1);
        RB_BindFragmentTextures(&tr.fragshader[FS_WATER],0,tr.waterImage->texnum,0,0,0,0,0,0);

        for (j=0;j<tess.numStaticSurfs;j++)
        {
                srf=tess.StaticSurfs[j];
                qglDrawElements(GL_TRIANGLES, srf->numIndices , GL_UNSIGNED_INT,  GEOMETRY_INDEX_OFFSET(srf->startIndex)  );
        }

}


void RB_BindFragmentTextures(fragShader_t *frag,int diffuse0,   int normal0,    int bump0,
                                                                                                int diffuse1,   int normal1,    int bump1,
                                                                                                int cubemapA,int cubemapB)
{

        int texnum=0;

        /*int diffuse0;
        int normal0;
        int bump0;
        int diffuse1;
        int normal1;
        int bump1;
        int lightmap0;
        int radbump0;
        int radbump1;
        int radbump2;
        int cubemap;
        int reflectmap;
        int refractmap;
        int scenetexture;*/

        if (frag->diffuse0!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );

                if (r_glsl_debug->integer>0)
                {
                        GL_BindTexnum(tr.greenImage->texnum);
                }
                else
                {
                        GL_BindTexnum(diffuse0);
                }
        }
        if (frag->normal0!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(normal0);
        }
        if (frag->bump0!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(bump0);
        }


        if (frag->diffuse1!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );

                if (r_glsl_debug->integer>0)
                {
                        GL_BindTexnum(tr.greenImage->texnum);
                }
                else
                {
                        GL_BindTexnum(diffuse1);
                }

        }
        if (frag->normal1!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(normal1);
        }
        if (frag->bump1!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(bump1);
        }

        if (frag->lightmap0!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );

                if (tess.shader->whiteLightmap==qtrue)
                {
                        GL_BindTexnum(tr.whiteImage->texnum);
                }
                else
                {
                        GL_BindTexnum(tr.fatlightmap->texnum);
                }
        }


        if (frag->radbump0!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.radbump[0]->texnum);
        }
        if (frag->radbump1!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.radbump[1]->texnum);
        }
        if (frag->radbump2!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.radbump[2]->texnum);
        }
        if (frag->specbump!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.radbump[3]->texnum);
        }

        if (frag->cubemap!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_CUBE_MAP );
                GL_BindCubeTexnum(cubemapA);
        }
        if (frag->cubemap2!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_CUBE_MAP );
                GL_BindCubeTexnum(cubemapB);
        }

        if (frag->reflectmap!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.reflectionTexnum);
        }
        if (frag->refractmap!=-1)
        {
                GL_SelectTexture(texnum++);
                qglEnable( GL_TEXTURE_2D );
                GL_BindTexnum(tr.underwaterTexnum);
        }

}

/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
        shaderCommands_t *input;

        input = &tess;

        //27 New static surface stuff
        if (input->numIndexes == 0 && tess.numStaticSurfs==0) {
                return;
        }

        if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
                ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
        }
        if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
                ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
        }

        if ( tess.shader == tr.shadowShader ) {
                RB_ShadowTessEnd();
                return;
        }

        // for debugging of sort order issues, stop rendering after a given sort value
        if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
                return;
        }

        //
        // update performance counters
        //
        backEnd.pc.c_shaders++;
        backEnd.pc.c_vertexes += tess.numVertexes;
        backEnd.pc.c_indexes += tess.numIndexes;
        backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

        //Vbo on or off?

        if (tess.numStaticSurfs==0)
        {
                RB_SetupFragment(NULL,tess.shader);
        }

        //
        // call off to shader specific tess end function
        //
        //Actual drawing!
        tess.currentStageIteratorFunc();

        //
        // draw debugging stuff
        //
        if(tess.numStaticSurfs==0)
        {
        if ( r_showtris->integer ) {
                DrawTris (input);
        }
        if ( r_shownormals->integer ) {
                DrawNormals (input);
        }
        }
        else
        {
                //Vbo version
                if ( r_showtris->integer ) {
                        DrawStaticTris (input);
                }

        }
        // clear shader so we can tell we don't have any unclosed surfaces
        tess.numIndexes = 0;

        tess.numStaticSurfs=0;

        GLimp_LogComment( "----------\n" );
}
