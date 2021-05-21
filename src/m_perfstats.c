// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file m_perfstats.c
/// \brief Performance measurement tools.

#include "m_perfstats.h"
#include "v_video.h"
#include "i_video.h"
#include "d_netcmd.h"
#include "r_main.h"
#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

int ps_tictime = 0;

int ps_playerthink_time = 0;
int ps_thinkertime = 0;

int ps_checkposition_calls = 0;

#ifdef HAVE_BLUA
int ps_lua_thinkframe_time = 0;
int ps_lua_mobjhooks = 0;

// dynamically allocated resizeable array for thinkframe hook stats
ps_hookinfo_t *thinkframe_hooks = NULL;
int thinkframe_hooks_length = 0;
int thinkframe_hooks_capacity = 16;

void PS_SetThinkFrameHookInfo(int index, UINT32 time_taken, char* short_src)
{
	if (!thinkframe_hooks)
	{
		// array needs to be initialized
		thinkframe_hooks = Z_Malloc(sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	if (index >= thinkframe_hooks_capacity)
	{
		// array needs more space, realloc with double size
		thinkframe_hooks_capacity *= 2;
		thinkframe_hooks = Z_Realloc(thinkframe_hooks,
			sizeof(ps_hookinfo_t) * thinkframe_hooks_capacity, PU_STATIC, NULL);
	}
	thinkframe_hooks[index].time_taken = time_taken;
	memcpy(thinkframe_hooks[index].short_src, short_src, LUA_IDSIZE * sizeof(char));
	// since the values are set sequentially from begin to end, the last call should leave
	// the correct value to this variable
	thinkframe_hooks_length = index + 1;
}
#endif

void M_DrawPerfStats(void)
{
	char s[100];
	int currenttime = I_GetTimeMicros();
	int frametime = currenttime - ps_prevframetime;
	int divisor = 1;
	ps_prevframetime = currenttime;

//	if (ps_rendercalltime > 10000)
//		divisor = 1000;
	if (cv_perfstats.value == 1) // rendering
	{
		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			snprintf(s, sizeof s - 1, "frmtime %d", frametime / divisor);
			V_DrawThinString(20, 10, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "drwtime %d", ps_rendercalltime / divisor);
			V_DrawThinString(20, 20, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "skybox  %d", ps_skyboxtime / divisor);
			V_DrawThinString(24, 30, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "bsptime %d", ps_bsptime / divisor);
			V_DrawThinString(24, 40, V_MONOSPACE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "bspcall %d", ps_numbspcalls);
			V_DrawThinString(90, 10, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "sprites %d", ps_numsprites);
			V_DrawThinString(90, 20, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "drwnode %d", ps_numdrawnodes);
			V_DrawThinString(90, 30, V_MONOSPACE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "plyobjs %d", ps_numpolyobjects);
			V_DrawThinString(90, 40, V_MONOSPACE | V_BLUEMAP, s);
	#ifdef HWRENDER
			if (rendermode == render_opengl) // OpenGL specific stats
			{
				snprintf(s, sizeof s - 1, "nodesrt %d", ps_hw_nodesorttime / divisor);
				V_DrawThinString(24, 50, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "nodedrw %d", ps_hw_nodedrawtime / divisor);
				V_DrawThinString(24, 60, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "sprsort %d", ps_hw_spritesorttime / divisor);
				V_DrawThinString(24, 70, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "sprdraw %d", ps_hw_spritedrawtime / divisor);
				V_DrawThinString(24, 80, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "other   %d",
					(ps_rendercalltime - ps_skyboxtime - ps_bsptime - ps_hw_nodesorttime
					- ps_hw_nodedrawtime - ps_hw_spritesorttime - ps_hw_spritedrawtime
					- ps_hw_batchsorttime - ps_hw_batchdrawtime) / divisor);
				V_DrawThinString(24, 90, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "ui      %d", ps_uitime / divisor);
				V_DrawThinString(20, 100, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "finupdt %d", ps_swaptime / divisor);
				V_DrawThinString(20, 110, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "tic     %d", ps_tictime / divisor);
				V_DrawThinString(20, 125, V_MONOSPACE | V_GRAYMAP, s);
				if (cv_grbatching.value)
				{
					snprintf(s, sizeof s - 1, "batsort %d", ps_hw_batchsorttime / divisor);
					V_DrawThinString(90, 55, V_MONOSPACE | V_REDMAP, s);
					snprintf(s, sizeof s - 1, "batdraw %d", ps_hw_batchdrawtime / divisor);
					V_DrawThinString(90, 65, V_MONOSPACE | V_REDMAP, s);

					snprintf(s, sizeof s - 1, "polygon %d", ps_hw_numpolys);
					V_DrawThinString(155, 10, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "drwcall %d", ps_hw_numcalls);
					V_DrawThinString(155, 20, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "shaders %d", ps_hw_numshaders);
					V_DrawThinString(155, 30, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "vertex  %d", ps_hw_numverts);
					V_DrawThinString(155, 40, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "texture %d", ps_hw_numtextures);
					V_DrawThinString(220, 10, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "polyflg %d", ps_hw_numpolyflags);
					V_DrawThinString(220, 20, V_MONOSPACE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "colors  %d", ps_hw_numcolors);
					V_DrawThinString(220, 30, V_MONOSPACE | V_PURPLEMAP, s);
				}
				else
				{
					// reset these vars so the "other" measurement isn't off
					ps_hw_batchsorttime = 0;
					ps_hw_batchdrawtime = 0;
				}
			}
			else // software specific stats
	#endif
			{
				snprintf(s, sizeof s - 1, "sprclip %d", ps_sw_spritecliptime / divisor);
				V_DrawThinString(24, 50, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "portals %d", ps_sw_portaltime / divisor);
				V_DrawThinString(24, 60, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "planes  %d", ps_sw_planetime / divisor);
				V_DrawThinString(24, 70, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "masked  %d", ps_sw_maskedtime / divisor);
				V_DrawThinString(24, 80, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "other   %d",
					(ps_rendercalltime - ps_skyboxtime - ps_bsptime - ps_sw_spritecliptime
					- ps_sw_portaltime - ps_sw_planetime - ps_sw_maskedtime) / divisor);
				V_DrawThinString(24, 90, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "ui      %d", ps_uitime / divisor);
				V_DrawThinString(20, 100, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "finupdt %d", ps_swaptime / divisor);
				V_DrawThinString(20, 110, V_MONOSPACE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "tic     %d", ps_tictime / divisor);
				V_DrawThinString(20, 125, V_MONOSPACE | V_GRAYMAP, s);
			}
		}
		else // high resolution
		{
			snprintf(s, sizeof s - 1, "Frame time:     %d", frametime / divisor);
			V_DrawSmallString(20, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "3d rendering:   %d", ps_rendercalltime / divisor);
			V_DrawSmallString(20, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "Skybox render:  %d", ps_skyboxtime / divisor);
			V_DrawSmallString(24, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "RenderBSPNode:  %d", ps_bsptime / divisor);
			V_DrawSmallString(24, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "BSP calls:    %d", ps_numbspcalls);
			V_DrawSmallString(115, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Sprites:      %d", ps_numsprites);
			V_DrawSmallString(115, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Drawnodes:    %d", ps_numdrawnodes);
			V_DrawSmallString(115, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Polyobjects:  %d", ps_numpolyobjects);
			V_DrawSmallString(115, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			#ifdef HWRENDER
			if (rendermode == render_opengl) // OpenGL specific stats
			{
				snprintf(s, sizeof s - 1, "Drwnode sort:   %d", ps_hw_nodesorttime / divisor);
				V_DrawSmallString(24, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Drwnode render: %d", ps_hw_nodedrawtime / divisor);
				V_DrawSmallString(24, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Sprite sort:    %d", ps_hw_spritesorttime / divisor);
				V_DrawSmallString(24, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Sprite render:  %d", ps_hw_spritedrawtime / divisor);
				V_DrawSmallString(24, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				// Remember to update this calculation when adding more 3d rendering stats!
				snprintf(s, sizeof s - 1, "Other:          %d",
					(ps_rendercalltime - ps_skyboxtime - ps_bsptime - ps_hw_nodesorttime
					- ps_hw_nodedrawtime - ps_hw_spritesorttime - ps_hw_spritedrawtime
					- ps_hw_batchsorttime - ps_hw_batchdrawtime) / divisor);
				V_DrawSmallString(24, 50, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "UI render:      %d", ps_uitime / divisor);
				V_DrawSmallString(20, 55, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "I_FinishUpdate: %d", ps_swaptime / divisor);
				V_DrawSmallString(20, 60, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Game logic:     %d", ps_tictime / divisor);
				V_DrawSmallString(20, 70, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
				if (cv_grbatching.value)
				{
					snprintf(s, sizeof s - 1, "Batch sort:   %d", ps_hw_batchsorttime / divisor);
					V_DrawSmallString(115, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_REDMAP, s);
					snprintf(s, sizeof s - 1, "Batch render: %d", ps_hw_batchdrawtime / divisor);
					V_DrawSmallString(115, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_REDMAP, s);

					snprintf(s, sizeof s - 1, "Polygons:   %d", ps_hw_numpolys);
					V_DrawSmallString(200, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Vertices:   %d", ps_hw_numverts);
					V_DrawSmallString(200, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Draw calls: %d", ps_hw_numcalls);
					V_DrawSmallString(200, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Shaders:    %d", ps_hw_numshaders);
					V_DrawSmallString(200, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Textures:   %d", ps_hw_numtextures);
					V_DrawSmallString(200, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Polyflags:  %d", ps_hw_numpolyflags);
					V_DrawSmallString(200, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
					snprintf(s, sizeof s - 1, "Colors:     %d", ps_hw_numcolors);
					V_DrawSmallString(200, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
				}
				else
				{
					// reset these vars so the "other" measurement isn't off
					ps_hw_batchsorttime = 0;
					ps_hw_batchdrawtime = 0;
				}
			}
			else // software specific stats
	#endif
			{
				snprintf(s, sizeof s - 1, "R_ClipSprites:  %d", ps_sw_spritecliptime / divisor);
				V_DrawSmallString(24, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Portal render:  %d", ps_sw_portaltime / divisor);
				V_DrawSmallString(24, 35, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "R_DrawPlanes:   %d", ps_sw_planetime / divisor);
				V_DrawSmallString(24, 40, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "R_DrawMasked:   %d", ps_sw_maskedtime / divisor);
				V_DrawSmallString(24, 45, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				// Remember to update this calculation when adding more 3d rendering stats!
				snprintf(s, sizeof s - 1, "Other:          %d",
					(ps_rendercalltime - ps_skyboxtime - ps_bsptime - ps_sw_spritecliptime
					- ps_sw_portaltime - ps_sw_planetime - ps_sw_maskedtime) / divisor);
				V_DrawSmallString(24, 50, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "UI render:      %d", ps_uitime / divisor);
				V_DrawSmallString(20, 55, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "I_FinishUpdate: %d", ps_swaptime / divisor);
				V_DrawSmallString(20, 60, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
				snprintf(s, sizeof s - 1, "Game logic:     %d", ps_tictime / divisor);
				V_DrawSmallString(20, 70, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
			}
		}
	}
	else if (cv_perfstats.value == 2) // logic
	{
		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			V_DrawThinString(30, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "Not implemented for low res");
		}
		else // high resolution
		{
			thinker_t *thinker;
			int thinkercount = 0;
			int mobjcount = 0;
			// These seem to be very rare since they don't get thinkers on spawn.
			// Could someone theoretically set MF_NOTHINK during runtime though?
			int nothinkcount = 0;
			int scenerycount = 0;
			int precipcount = 0;
			int removecount = 0;
			// y offset for drawing second column
			int yoffset2 = 0;
			snprintf(s, sizeof s - 1, "Game logic:      %d", ps_tictime);
			V_DrawSmallString(20, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "P_PlayerThink:   %d", ps_playerthink_time);
			V_DrawSmallString(24, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "P_RunThinkers:   %d", ps_thinkertime);
			V_DrawSmallString(24, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
#ifdef HAVE_BLUA
			snprintf(s, sizeof s - 1, "LUAh_ThinkFrame: %d", ps_lua_thinkframe_time);
			V_DrawSmallString(24, 25, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
			snprintf(s, sizeof s - 1, "Other:           %d",
				ps_tictime - ps_playerthink_time - ps_thinkertime - ps_lua_thinkframe_time);
			V_DrawSmallString(24, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, s);
#endif
			for (thinker = thinkercap.next; thinker != &thinkercap; thinker = thinker->next)
			{
				thinkercount++;
				if (thinker->function.acp1 == (actionf_p1)P_MobjThinker)
				{
					mobj_t *mobj = (mobj_t*)thinker;
					mobjcount++;
					if (mobj->flags & MF_NOTHINK)
						nothinkcount++;
					else if (mobj->flags & MF_SCENERY)
						scenerycount++;
				}
				else if (thinker->function.acp1 == (actionf_p1)P_NullPrecipThinker)
					precipcount++;
				else if (thinker->function.acp1 == (actionf_p1)P_RemoveThinkerDelayed)
					removecount++;
			}
			snprintf(s, sizeof s - 1, "Thinkers:        %d", thinkercount);
			V_DrawSmallString(115, 10+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Mobjs:           %d", mobjcount);
			V_DrawSmallString(119, 15+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Regular:         %d", mobjcount - scenerycount - nothinkcount);
			V_DrawSmallString(123, 20+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Scenery:         %d", scenerycount);
			V_DrawSmallString(123, 25+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			if (nothinkcount)
			{
				snprintf(s, sizeof s - 1, "Nothink:         %d", nothinkcount);
				V_DrawSmallString(123, 30+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
				yoffset2 += 5;
			}
			snprintf(s, sizeof s - 1, "Precip:          %d", precipcount);
			V_DrawSmallString(119, 30+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Pending removal: %d", removecount);
			V_DrawSmallString(119, 35+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);
			snprintf(s, sizeof s - 1, "Other:           %d", thinkercount - mobjcount - precipcount - removecount);
			V_DrawSmallString(119, 40+yoffset2, V_MONOSPACE | V_ALLOWLOWERCASE | V_BLUEMAP, s);

			snprintf(s, sizeof s - 1, "Calls:");
			V_DrawSmallString(212, 10, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
#ifdef HAVE_BLUA
			snprintf(s, sizeof s - 1, "Lua mobj hooks:  %d", ps_lua_mobjhooks);
			V_DrawSmallString(216, 15, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
#endif
			snprintf(s, sizeof s - 1, "P_CheckPosition: %d", ps_checkposition_calls);
			V_DrawSmallString(216, 20, V_MONOSPACE | V_ALLOWLOWERCASE | V_PURPLEMAP, s);
		}
	}
#ifdef HAVE_BLUA
	else if (cv_perfstats.value == 3) // lua thinkframe
	{
		if (vid.width < 640 || vid.height < 400) // low resolution
		{
			V_DrawThinString(30, 30, V_MONOSPACE | V_ALLOWLOWERCASE | V_YELLOWMAP, "Not implemented for low res");
		}
		else // high resolution
		{
			int i;
			// text writing position
			int x = 2;
			int y = 4;
			UINT32 text_color;
			char tempbuffer[LUA_IDSIZE];
			char last_mod_name[LUA_IDSIZE];
			last_mod_name[0] = '\0';
			for (i = 0; i < thinkframe_hooks_length; i++)
			{
				char* str = thinkframe_hooks[i].short_src;
				char* tempstr = tempbuffer;
				int len = (int)strlen(str);
				char* str_ptr;
				if (strcmp(".lua", str + len - 4) == 0)
				{
					str[len-4] = '\0'; // remove .lua at end
					len -= 4;
				}
				// we locate the wad/pk3 name in the string and compare it to
				// what we found on the previous iteration.
				// if the name has changed, print it out on the screen
				strcpy(tempstr, str);
				str_ptr = strrchr(tempstr, '|');
				if (str_ptr)
				{
					*str_ptr = '\0';
					str = str_ptr + 1; // this is the name of the hook without the mod file name
					str_ptr = strrchr(tempstr, '/');
					if (str_ptr)
						tempstr = str_ptr + 1;
					// tempstr should now point to the mod name, (wad/pk3) possibly truncated
					if (strcmp(tempstr, last_mod_name) != 0)
					{
						strcpy(last_mod_name, tempstr);
						len = (int)strlen(tempstr);
						if (len > 25)
							tempstr += len - 25;
						snprintf(s, sizeof s - 1, "%s", tempstr);
						V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | V_GRAYMAP, s);
						y += 4; // repeated code!
						if (y > 192)
						{
							y = 4;
							x += 106;
							if (x > 214)
								break;
						}
					}
					text_color = V_YELLOWMAP;
				}
				else
				{
					// probably a standalone lua file
					// cut off the folder if it's there
					str_ptr = strrchr(tempstr, '/');
					if (str_ptr)
						str = str_ptr + 1;
					text_color = 0; // white
				}
				len = (int)strlen(str);
				if (len > 20)
					str += len - 20;
				snprintf(s, sizeof s - 1, "%20s: %u", str, thinkframe_hooks[i].time_taken);
				V_DrawSmallString(x, y, V_MONOSPACE | V_ALLOWLOWERCASE | text_color, s);
				y += 4;
				if (y > 192)
				{
					y = 4;
					x += 106;
					if (x > 214)
						break;
				}
			}
		}
	}
#endif
}
