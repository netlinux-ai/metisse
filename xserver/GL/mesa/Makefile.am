INCLUDES = 						\
	-I$(top_srcdir)/xserver/include			\
	-I$(top_srcdir)/xserver/GL/glx			\
	-I$(top_srcdir)/xserver/GL/include		\
	-I$(top_srcdir)/xserver/GL/mesa/include		\
	-I$(top_srcdir)/xserver/GL/mesa/X		\
	-I$(top_srcdir)/xserver/GL/mesa			\
	-I$(top_srcdir)/mi				\
	-I$(top_srcdir)/Xext				\
	-I$(top_srcdir)/render				\
	-I$(top_srcdir)/xserver/GL/mesa/		\
	-I$(top_srcdir)/xserver/GL/mesa/main		\
	-I$(top_srcdir)/xserver/GL/mesa/glapi		\
	-I$(top_srcdir)/xserver/GL/mesa/drivers/x11 	\
	$(XSERVER_CFLAGS)

noinst_LIBRARIES = libglcore.a

COREMESABASESRCS = \
	main/accum.c  \
	main/accum.h  \
	main/api_arrayelt.c  \
	main/api_arrayelt.h  \
	main/api_eval.h  \
	main/api_loopback.c  \
	main/api_loopback.h  \
	main/api_noop.c  \
	main/api_noop.h  \
	main/api_validate.c  \
	main/api_validate.h  \
	main/arbfragparse.c \
	main/arbfragparse.h \
	main/arbparse.c \
	main/arbparse.h \
	main/arbparse_syn.h \
	main/arbprogram.c \
	main/arbprogram.h \
	main/arbvertparse.c \
	main/arbvertparse.h \
	main/attrib.c  \
	main/attrib.h  \
	main/blend.c  \
	main/blend.h  \
	main/buffers.c  \
	main/buffers.h  \
	main/bufferobj.c  \
	main/bufferobj.h \
	main/clip.c  \
	main/clip.h  \
	main/colormac.h  \
	main/colortab.c  \
	main/colortab.h  \
	main/config.h  \
	main/context.c  \
	main/context.h  \
	main/convolve.c  \
	main/convolve.h  \
	main/dd.h  \
	main/debug.c  \
	main/debug.h  \
	main/depth.c  \
	main/depth.h  \
	main/dispatch.c  \
	main/dlist.c  \
	main/dlist.h  \
	main/drawpix.c  \
	main/drawpix.h  \
	main/enable.c  \
	main/enable.h  \
	main/enums.c  \
	main/enums.h  \
	main/eval.c  \
	main/eval.h  \
	main/extensions.c  \
	main/extensions.h  \
	main/feedback.c  \
	main/feedback.h  \
	main/fog.c  \
	main/fog.h  \
	main/get.c  \
	main/get.h  \
	main/glheader.h \
	main/hash.c  \
	main/hash.h  \
	main/hint.c  \
	main/hint.h  \
	main/histogram.c  \
	main/histogram.h  \
	main/image.c  \
	main/image.h  \
	main/imports.c  \
	main/imports.h  \
	main/light.c  \
	main/light.h  \
	main/lines.c  \
	main/lines.h  \
	main/macros.h  \
	main/matrix.c  \
	main/matrix.h  \
	main/mtypes.h  \
	main/nvfragparse.c \
	main/nvfragparse.h \
	main/nvfragprog.h \
	main/nvprogram.c \
	main/nvprogram.h \
	main/nvvertexec.c \
	main/nvvertexec.h \
	main/nvvertparse.c \
	main/nvvertparse.h \
	main/nvvertprog.h \
	main/occlude.c \
	main/occlude.h \
	main/pixel.c  \
	main/pixel.h  \
	main/points.c  \
	main/points.h  \
	main/polygon.c  \
	main/polygon.h  \
	main/program.c \
	main/program.h \
	main/rastpos.c  \
	main/rastpos.h  \
	main/simple_list.h  \
	main/state.c  \
	main/state.h  \
	main/stencil.c  \
	main/stencil.h  \
	main/texcompress.c  \
	main/texcompress.h  \
	main/texformat.c  \
	main/texformat.h  \
	main/texformat_tmp.h  \
	main/teximage.c  \
	main/teximage.h  \
	main/texobj.c  \
	main/texobj.h  \
	main/texstate.c  \
	main/texstate.h  \
	main/texstore.c  \
	main/texstore.h  \
	main/texutil.c  \
	main/texutil.h  \
	main/texutil_tmp.h  \
	main/varray.c  \
	main/varray.h  \
	main/version.h \
	main/vtxfmt.c  \
	main/vtxfmt.h  \
	main/vtxfmt_tmp.h  \
	glapi/glapi.c \
	glapi/glapi.h \
	glapi/glapioffsets.h \
	glapi/glapitable.h \
	glapi/glapitemp.h \
	glapi/glprocs.h \
	glapi/glthread.c \
	glapi/glthread.h 

# disapear in 5 -> 6
# 	main/api_eval.c
#	main/vpexec.c  
#	main/vpexec.h  
#	main/vpparse.c  
#	main/vpparse.h  
#	main/vpstate.c  
#	main/vpstate.h  
#	main/mmath.c
#	main/mmath.h

#	main/arbparse.c 
#	main/arbprogram.c 
#	main/arbfragparse.c 
#	main/arbvertparse.c 
#	main/bufferobj.c
#	main/nvprogram.c
#	main/nvfragparse.c
#	main/nvvertexec.c
#	main/nvvertparse.c
#	main/occlude.c
#	main/program.c

MESA_AC_SRCS = \
	array_cache/acache.h \
	array_cache/ac_context.c \
	array_cache/ac_context.h \
	array_cache/ac_import.c

MESA_SWRAST_SRCS = \
	swrast/s_aaline.c \
	swrast/s_aaline.h \
	swrast/s_aalinetemp.h \
	swrast/s_aatriangle.c \
	swrast/s_aatriangle.h \
	swrast/s_aatritemp.h \
	swrast/s_accum.c \
	swrast/s_accum.h \
	swrast/s_alphabuf.c \
	swrast/s_alphabuf.h \
	swrast/s_alpha.c \
	swrast/s_alpha.h \
	swrast/s_bitmap.c \
	swrast/s_blend.c \
	swrast/s_blend.h \
	swrast/s_buffers.c \
	swrast/s_context.c \
	swrast/s_context.h \
	swrast/s_copypix.c \
	swrast/s_depth.c \
	swrast/s_depth.h \
	swrast/s_drawpix.c \
	swrast/s_drawpix.h \
	swrast/s_feedback.c \
	swrast/s_feedback.h \
	swrast/s_fog.c \
	swrast/s_fog.h \
	swrast/s_imaging.c \
	swrast/s_lines.c \
	swrast/s_lines.h \
	swrast/s_linetemp.h \
	swrast/s_logic.c \
	swrast/s_logic.h \
	swrast/s_masking.c \
	swrast/s_masking.h \
	swrast/s_nvfragprog.c \
	swrast/s_nvfragprog.h \
	swrast/s_pixeltex.c \
	swrast/s_pixeltex.h \
	swrast/s_points.c \
	swrast/s_points.h \
	swrast/s_pointtemp.h \
	swrast/s_readpix.c \
	swrast/s_span.c \
	swrast/s_span.h \
	swrast/s_stencil.c \
	swrast/s_stencil.h \
	swrast/s_texstore.c \
	swrast/s_texture.c \
	swrast/s_texture.h \
	swrast/s_triangle.c \
	swrast/s_triangle.h \
	swrast/s_trispan.h \
	swrast/s_tritemp.h \
	swrast/swrast.h \
	swrast/s_zoom.c \
	swrast/s_zoom.h

#	swrast/s_histogram.c
#	swrast/s_histogram.h

#	swrast/s_nvfragprog.c

MESA_SWR_SETUP_SRCS = \
	swrast_setup/ss_context.c \
	swrast_setup/ss_context.h \
	swrast_setup/ss_triangle.c \
	swrast_setup/ss_triangle.h \
	swrast_setup/ss_tritmp.h \
	swrast_setup/ss_vb.h \
	swrast_setup/swrast_setup.h

#	swrast_setup/ss_vb.c
#	swrast_setup/ss_vbtmp.h

MESA_TNL_SRCS = \
	tnl/t_array_api.c \
	tnl/t_array_api.h \
	tnl/t_array_import.c \
	tnl/t_array_import.h \
	tnl/t_context.c \
	tnl/t_context.h \
	tnl/tnl.h \
	tnl/t_pipeline.c \
	tnl/t_pipeline.h \
	tnl/t_save_api.c \
	tnl/t_save_api.h \
	tnl/t_save_loopback.c \
	tnl/t_save_playback.c \
	tnl/t_vb_cliptmp.h \
	tnl/t_vb_fog.c \
	tnl/t_vb_light.c \
	tnl/t_vb_lighttmp.h \
	tnl/t_vb_normals.c \
	tnl/t_vb_points.c \
	tnl/t_vb_program.c \
	tnl/t_vb_render.c \
	tnl/t_vb_rendertmp.h \
	tnl/t_vb_texgen.c \
	tnl/t_vb_texmat.c \
	tnl/t_vb_vertex.c \
	tnl/t_vertex.c \
	tnl/t_vertex.h \
	tnl/t_vtx_api.c \
	tnl/t_vtx_api.h \
	tnl/t_vtx_eval.c \
	tnl/t_vtx_exec.c

#	tnl/t_eval_api.c
#	tnl/t_eval_api.h
#	tnl/t_imm_alloc.c
#	tnl/t_imm_alloc.h
#	tnl/t_imm_api.c
#	tnl/t_imm_api.h
#	tnl/t_imm_debug.c
#	tnl/t_imm_debug.h
#	tnl/t_imm_dlist.c
#	tnl/t_imm_dlist.h
#	tnl/t_imm_elt.c
#	tnl/t_imm_elt.h
#	tnl/t_imm_eval.c
#	tnl/t_imm_eval.h
#	tnl/t_imm_exec.c
#	tnl/t_imm_exec.h
#	tnl/t_imm_fixup.c
#	tnl/t_imm_fixup.h

#	tnl/t_vtx_exec.c
#	tnl/t_vtx_sse.c
#	tnl/t_vtx_x86.c
#	tnl/t_vtx_api.c
#	tnl/t_vtx_api.h 

MESA_MATH_SRCS = \
	math/mathmod.h \
	math/m_clip_tmp.h \
	math/m_copy_tmp.h \
	math/m_debug_clip.c \
	math/m_debug.h \
	math/m_debug_norm.c \
	math/m_debug_util.h \
	math/m_debug_xform.c \
	math/m_dotprod_tmp.h \
	math/m_matrix.c \
	math/m_matrix.h \
	math/m_norm_tmp.h \
	math/m_translate.c \
	math/m_translate.h \
	math/m_trans_tmp.h \
	math/m_vector.c \
	math/m_vector.h \
	math/m_xform.c \
	math/m_xform.h \
	math/m_xform_tmp.h \
	math/m_eval.c \
	math/m_eval.h

XMESA_SRCS = \
	drivers/x11/glxapi.c \
	drivers/x11/glxapi.h \
	drivers/x11/glxheader.h \
	drivers/x11/realglx.c \
	drivers/x11/realglx.h \
	drivers/x11/xm_api.c \
	drivers/x11/xm_dd.c \
	drivers/x11/xmesaP.h \
	drivers/x11/xm_line.c \
	drivers/x11/xm_span.c \
	drivers/x11/xm_tri.c

# 	drivers/x11/fakeglx.c
#	drivers/x11/xfonts.c
#	drivers/x11/xfonts.h

MESA_COMMON_SRCS =
#MESA_COMMON_SRCS = \
#	drivers/common/driverfuncs.c

if GLX_X86
MESA_X86 = \
	X86/3dnow.c \
	X86/3dnow.h \
	X86/3dnow_normal.S \
	X86/3dnow_xform1.S \
	X86/3dnow_xform2.S \
	X86/3dnow_xform3.S \
	X86/3dnow_xform4.S \
	X86/assyntax.h \
	X86/clip_args.h \
	X86/common_x86_asm.h \
	X86/common_x86_asm.S \
	X86/common_x86.c \
	X86/common_x86_features.h \
	X86/common_x86_macros.h \
	X86/gen_matypes.c \
	X86/glapi_x86.S \
	X86/mmx_blend.S \
	X86/mmx_blendtmp.h \
	X86/mmx.h \
	X86/norm_args.h \
	X86/sse.c \
	X86/sse.h \
	X86/sse_normal.S \
	X86/sse_xform1.S \
	X86/sse_xform2.S \
	X86/sse_xform3.S \
	X86/sse_xform4.S \
	X86/x86.c \
	X86/x86_cliptest.S \
	X86/x86.h \
	X86/x86_xform2.S \
	X86/x86_xform3.S \
	X86/x86_xform4.S \
	X86/xform_args.h

# need some special rules here, unfortunately
X86/matypes.h: main/mtypes.h tnl/t_context.h X86/gen_matypes.c
	$(CC) -UHAVE_CONFIG_H $(INCLUDES) $(CFLAGS) X86/gen_matypes.c -o X86/gen_matypes
	X86/gen_matypes > X86/matypes.h

X86/common_x86_asm.S: X86/matypes.h
X86/3dnow_normal.S: X86/matypes.h
X86/3dnow_xform1.S: X86/matypes.h
X86/3dnow_xform2.S: X86/matypes.h
X86/3dnow_xform3.S: X86/matypes.h
X86/3dnow_xform4.S: X86/matypes.h
X86/mmx_blend.S: X86/matypes.h
X86/sse_normal.S: X86/matypes.h
X86/sse_xform1.S: X86/matypes.h
X86/sse_xform2.S: X86/matypes.h
X86/sse_xform3.S: X86/matypes.h
X86/sse_xform4.S: X86/matypes.h
X86/x86_cliptest.S: X86/matypes.h
X86/x86_xform2.S: X86/matypes.h
X86/x86_xform3.S: X86/matypes.h
X86/x86_xform4.S: X86/matypes.h
endif

X_SRCS = \
	X/xf86glx.c \
	X/xf86glx_util.c \
	X/xf86glx_util.h \
	X/xf86glxint.h \
	X/compsize.c \
	X/size.h 

libglcore_a_SOURCES = \
	$(X_SRCS) \
	$(COREMESABASESRCS) \
	$(MESA_AC_SRCS) \
	$(MESA_SWRAST_SRCS) \
	$(MESA_SWR_SETUP_SRCS) \
	$(MESA_TNL_SRCS) \
	$(MESA_MATH_SRCS) \
	$(MESA_COMMON_SRCS) \
	$(XMESA_SRCS) \
	$(MESA_X86)

# $(XMESA_SRCS)
#ASM_DEFINE = -DUSE_SSE_ASM -DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM \
#	-DX86_FXSR_MAGIC

if GLX_X86
ASM_DEFINE = -DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM
endif

#CFLAGS = -g -O3
# -D_POSIX_SOURCE  -DPTHREADS

libglcore_a_CFLAGS = -UHAVE_CONFIG_H -DXFree86Server -D_POSIX_SOURCE \
	-D_POSIX_C_SOURCE=199309L -D_SVID_SOURCE -D_BSD_SOURCE $(ASM_DEFINE)

# -DPTHREADS

# -DGLXEXT -DXF86DRI -DGLX_DIRECT_RENDERING -DGLX_USE_MESA
# -DGLX_USE_DLOPEN -DGLX_USE_MESA

AM_CCASFLAGS = $(libglcore_a_CFLAGS) $(INCLUDES)

CLEANFILES = X86/matypes.h X86/gen_matypes
