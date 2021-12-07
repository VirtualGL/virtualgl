/*
 * Copyright (C) 1999-2014  Brian Paul   All Rights Reserved.
 * Copyright (C) 2005  Sun Microsystems, Inc.   All Rights Reserved.
 * Copyright (C) 2014, 2018-2019  D. R. Commander   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glinfo_common.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif
#ifndef GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB 0x00000004
#endif
#ifndef GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008
#endif
#ifndef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#endif

/**
 * Return the GL enum name for a numeric value.
 * We really only care about the compressed texture formats for now.
 */
static const char *
enum_name(GLenum val)
{
   static const struct {
      const char *name;
      GLenum val;
   } enums [] = {
      { "GL_COMPRESSED_ALPHA", 0x84E9 },
      { "GL_COMPRESSED_LUMINANCE", 0x84EA },
      { "GL_COMPRESSED_LUMINANCE_ALPHA", 0x84EB },
      { "GL_COMPRESSED_INTENSITY", 0x84EC },
      { "GL_COMPRESSED_RGB", 0x84ED },
      { "GL_COMPRESSED_RGBA", 0x84EE },
      { "GL_COMPRESSED_TEXTURE_FORMATS", 0x86A3 },
      { "GL_COMPRESSED_RGB", 0x84ED },
      { "GL_COMPRESSED_RGBA", 0x84EE },
      { "GL_COMPRESSED_TEXTURE_FORMATS", 0x86A3 },
      { "GL_COMPRESSED_ALPHA", 0x84E9 },
      { "GL_COMPRESSED_LUMINANCE", 0x84EA },
      { "GL_COMPRESSED_LUMINANCE_ALPHA", 0x84EB },
      { "GL_COMPRESSED_INTENSITY", 0x84EC },
      { "GL_COMPRESSED_SRGB", 0x8C48 },
      { "GL_COMPRESSED_SRGB_ALPHA", 0x8C49 },
      { "GL_COMPRESSED_SLUMINANCE", 0x8C4A },
      { "GL_COMPRESSED_SLUMINANCE_ALPHA", 0x8C4B },
      { "GL_COMPRESSED_RED", 0x8225 },
      { "GL_COMPRESSED_RG", 0x8226 },
      { "GL_COMPRESSED_RED_RGTC1", 0x8DBB },
      { "GL_COMPRESSED_SIGNED_RED_RGTC1", 0x8DBC },
      { "GL_COMPRESSED_RG_RGTC2", 0x8DBD },
      { "GL_COMPRESSED_SIGNED_RG_RGTC2", 0x8DBE },
      { "GL_COMPRESSED_RGB8_ETC2", 0x9274 },
      { "GL_COMPRESSED_SRGB8_ETC2", 0x9275 },
      { "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2", 0x9276 },
      { "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2", 0x9277 },
      { "GL_COMPRESSED_RGBA8_ETC2_EAC", 0x9278 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC", 0x9279 },
      { "GL_COMPRESSED_R11_EAC", 0x9270 },
      { "GL_COMPRESSED_SIGNED_R11_EAC", 0x9271 },
      { "GL_COMPRESSED_RG11_EAC", 0x9272 },
      { "GL_COMPRESSED_SIGNED_RG11_EAC", 0x9273 },
      { "GL_COMPRESSED_ALPHA_ARB", 0x84E9 },
      { "GL_COMPRESSED_LUMINANCE_ARB", 0x84EA },
      { "GL_COMPRESSED_LUMINANCE_ALPHA_ARB", 0x84EB },
      { "GL_COMPRESSED_INTENSITY_ARB", 0x84EC },
      { "GL_COMPRESSED_RGB_ARB", 0x84ED },
      { "GL_COMPRESSED_RGBA_ARB", 0x84EE },
      { "GL_COMPRESSED_TEXTURE_FORMATS_ARB", 0x86A3 },
      { "GL_COMPRESSED_RGBA_BPTC_UNORM_ARB", 0x8E8C },
      { "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB", 0x8E8D },
      { "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB", 0x8E8E },
      { "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB", 0x8E8F },
      { "GL_COMPRESSED_RGBA_ASTC_4x4_KHR", 0x93B0 },
      { "GL_COMPRESSED_RGBA_ASTC_5x4_KHR", 0x93B1 },
      { "GL_COMPRESSED_RGBA_ASTC_5x5_KHR", 0x93B2 },
      { "GL_COMPRESSED_RGBA_ASTC_6x5_KHR", 0x93B3 },
      { "GL_COMPRESSED_RGBA_ASTC_6x6_KHR", 0x93B4 },
      { "GL_COMPRESSED_RGBA_ASTC_8x5_KHR", 0x93B5 },
      { "GL_COMPRESSED_RGBA_ASTC_8x6_KHR", 0x93B6 },
      { "GL_COMPRESSED_RGBA_ASTC_8x8_KHR", 0x93B7 },
      { "GL_COMPRESSED_RGBA_ASTC_10x5_KHR", 0x93B8 },
      { "GL_COMPRESSED_RGBA_ASTC_10x6_KHR", 0x93B9 },
      { "GL_COMPRESSED_RGBA_ASTC_10x8_KHR", 0x93BA },
      { "GL_COMPRESSED_RGBA_ASTC_10x10_KHR", 0x93BB },
      { "GL_COMPRESSED_RGBA_ASTC_12x10_KHR", 0x93BC },
      { "GL_COMPRESSED_RGBA_ASTC_12x12_KHR", 0x93BD },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR", 0x93D0 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR", 0x93D1 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR", 0x93D2 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR", 0x93D3 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR", 0x93D4 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR", 0x93D5 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR", 0x93D6 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR", 0x93D7 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR", 0x93D8 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR", 0x93D9 },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR", 0x93DA },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR", 0x93DB },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR", 0x93DC },
      { "GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR", 0x93DD },
      { "GL_COMPRESSED_RGB_FXT1_3DFX", 0x86B0 },
      { "GL_COMPRESSED_RGBA_FXT1_3DFX", 0x86B1 },
      { "GL_COMPRESSED_LUMINANCE_LATC1_EXT", 0x8C70 },
      { "GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT", 0x8C71 },
      { "GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT", 0x8C72 },
      { "GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT", 0x8C73 },
      { "GL_COMPRESSED_RED_RGTC1_EXT", 0x8DBB },
      { "GL_COMPRESSED_SIGNED_RED_RGTC1_EXT", 0x8DBC },
      { "GL_COMPRESSED_RED_GREEN_RGTC2_EXT", 0x8DBD },
      { "GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT", 0x8DBE },
      { "GL_COMPRESSED_RGB_S3TC_DXT1_EXT", 0x83F0 },
      { "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT", 0x83F1 },
      { "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT", 0x83F2 },
      { "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT", 0x83F3 },
      { "GL_COMPRESSED_SRGB_EXT", 0x8C48 },
      { "GL_COMPRESSED_SRGB_ALPHA_EXT", 0x8C49 },
      { "GL_COMPRESSED_SLUMINANCE_EXT", 0x8C4A },
      { "GL_COMPRESSED_SLUMINANCE_ALPHA_EXT", 0x8C4B },
      { "GL_COMPRESSED_SRGB_S3TC_DXT1_EXT", 0x8C4C },
      { "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT", 0x8C4D },
      { "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT", 0x8C4E },
      { "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT", 0x8C4F },
      { "GL_PALETTE4_RGB8_OES", 0x8B90 },
      { "GL_PALETTE4_RGBA8_OES", 0x8B91 },
      { "GL_PALETTE4_R5_G6_B5_OES", 0x8B92 },
      { "GL_PALETTE4_RGBA4_OES", 0x8B93 },
      { "GL_PALETTE4_RGB5_A1_OES", 0x8B94 },
      { "GL_PALETTE8_RGB8_OES", 0x8B95 },
      { "GL_PALETTE8_RGBA8_OES", 0x8B96 },
      { "GL_PALETTE8_R5_G6_B5_OES", 0x8B97 },
      { "GL_PALETTE8_RGBA4_OES", 0x8B98 },
      { "GL_PALETTE8_RGB5_A1_OES", 0x8B99 }
   };
   const int n = sizeof(enums) / sizeof(enums[0]);
   static char buffer[100];
   int i;
   for (i = 0; i < n; i++) {
      if (enums[i].val == val) {
         return enums[i].name;
      }
   }
   /* enum val not found, just print hexadecimal value into static buffer */
   snprintf(buffer, sizeof(buffer), "0x%x", val);
   return buffer;
}


/*
 * qsort callback for string comparison.
 */
static int
compare_string_ptr(const void *p1, const void *p2)
{
   return strcmp(* (char * const *) p1, * (char * const *) p2);
}

/*
 * Print a list of extensions, with word-wrapping.
 */
void
print_extension_list(const char *ext, GLboolean singleLine)
{
   char **extensions;
   int num_extensions;
   const char *indentString = "    ";
   const int indent = 4;
   const int max = 79;
   int width, i, j, k;

   if (!ext || !ext[0])
      return;

   /* count the number of extensions, ignoring successive spaces */
   num_extensions = 0;
   j = 1;
   do {
      if ((ext[j] == ' ' || ext[j] == 0) && ext[j - 1] != ' ') {
         ++num_extensions;
      }
   } while(ext[j++]);

   /* copy individual extensions to an array */
   extensions = malloc(num_extensions * sizeof *extensions);
   if (!extensions) {
      fprintf(stderr, "Error: malloc() failed\n");
      exit(1);
   }
   i = j = k = 0;
   while (1) {
      if (ext[j] == ' ' || ext[j] == 0) {
         /* found end of an extension name */
         const int len = j - i;

         if (len) {
            assert(k < num_extensions);

            extensions[k] = malloc(len + 1);
            if (!extensions[k]) {
               fprintf(stderr, "Error: malloc() failed\n");
               exit(1);
            }

            memcpy(extensions[k], ext + i, len);
            extensions[k][len] = 0;

            ++k;
         };

         i += len + 1;

         if (ext[j] == 0) {
            break;
         }
      }
      j++;
   }
   assert(k == num_extensions);

   /* sort extensions alphabetically */
   qsort(extensions, num_extensions, sizeof extensions[0], compare_string_ptr);

   /* print the extensions */
   width = indent;
   printf("%s", indentString);
   for (k = 0; k < num_extensions; ++k) {
      const int len = strlen(extensions[k]);
      if ((!singleLine) && (width + len > max)) {
         /* start a new line */
         printf("\n");
         width = indent;
         printf("%s", indentString);
      }
      /* print the extension name */
      printf("%s", extensions[k]);

      /* either we're all done, or we'll continue with next extension */
      width += len + 1;

      if (singleLine) {
         printf("\n");
         width = indent;
         printf("%s", indentString);
      }
      else if (k < (num_extensions -1)) {
         printf(", ");
         width += 2;
      }
   }
   printf("\n");

   for (k = 0; k < num_extensions; ++k) {
      free(extensions[k]);
   }
   free(extensions);
}




/**
 * Get list of OpenGL extensions using core profile's glGetStringi().
 */
char *
build_core_profile_extension_list(const struct ext_functions *extfuncs)
{
   GLint i, n, totalLen;
   char *buffer;

   glGetIntegerv(GL_NUM_EXTENSIONS, &n);

   /* compute totalLen */
   totalLen = 0;
   for (i = 0; i < n; i++) {
      const char *ext = (const char *) extfuncs->GetStringi(GL_EXTENSIONS, i);
      if (ext)
          totalLen += strlen(ext) + 1; /* plus a space */
   }

   if (!totalLen)
     return NULL;

   buffer = malloc(totalLen + 1);
   if (buffer) {
      int pos = 0;
      for (i = 0; i < n; i++) {
         const char *ext = (const char *) extfuncs->GetStringi(GL_EXTENSIONS, i);
         strcpy(buffer + pos, ext);
         pos += strlen(ext);
         buffer[pos++] = ' ';
      }
      buffer[pos] = '\0';
   }
   return buffer;
}


/** Is extension 'ext' supported? */
GLboolean
extension_supported(const char *ext, const char *extensionsList)
{
   while (1) {
      const char *p = strstr(extensionsList, ext);
      if (p) {
         /* check that next char is a space or end of string */
         int extLen = strlen(ext);
         if (p[extLen] == 0 || p[extLen] == ' ') {
            return 1;
         }
         else {
            /* We found a superset string, keep looking */
            extensionsList += extLen;
         }
      }
      else {
         break;
      }
   }
   return 0;
}


/**
 * Is verNum >= verString?
 * \param verString  such as "2.1", "3.0", etc.
 * \param verNum  such as 20, 21, 30, 31, 32, etc.
 */
static GLboolean
version_supported(const char *verString, int verNum)
{
   int v;

   if (!verString ||
       !isdigit(verString[0]) ||
       verString[1] != '.' ||
       !isdigit(verString[2])) {
      return GL_FALSE;
   }

   v = (verString[0] - '0') * 10 + (verString[2] - '0');

   return verNum >= v;
}


struct token_name
{
   GLenum token;
   const char *name;
};


static void
print_shader_limit_list(const struct token_name *lim)
{
   GLint max[1];
   unsigned i;

   for (i = 0; lim[i].token; i++) {
      glGetIntegerv(lim[i].token, max);
      if (glGetError() == GL_NO_ERROR) {
         printf("        %s = %d\n", lim[i].name, max[0]);
      }
   }
}


/**
 * Print interesting limits for vertex/fragment shaders.
 */
static void
print_shader_limits(GLenum target)
{
   static const struct token_name vertex_limits[] = {
      { GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB, "GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB" },
      { GL_MAX_VARYING_FLOATS_ARB, "GL_MAX_VARYING_FLOATS_ARB" },
      { GL_MAX_VERTEX_ATTRIBS_ARB, "GL_MAX_VERTEX_ATTRIBS_ARB" },
      { GL_MAX_TEXTURE_IMAGE_UNITS_ARB, "GL_MAX_TEXTURE_IMAGE_UNITS_ARB" },
      { GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB, "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB" },
      { GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB, "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB" },
      { GL_MAX_TEXTURE_COORDS_ARB, "GL_MAX_TEXTURE_COORDS_ARB" },
      { GL_MAX_VERTEX_OUTPUT_COMPONENTS  , "GL_MAX_VERTEX_OUTPUT_COMPONENTS  " },
      { (GLenum) 0, NULL }
   };
   static const struct token_name fragment_limits[] = {
      { GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB, "GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB" },
      { GL_MAX_TEXTURE_COORDS_ARB, "GL_MAX_TEXTURE_COORDS_ARB" },
      { GL_MAX_TEXTURE_IMAGE_UNITS_ARB, "GL_MAX_TEXTURE_IMAGE_UNITS_ARB" },
      { GL_MAX_FRAGMENT_INPUT_COMPONENTS , "GL_MAX_FRAGMENT_INPUT_COMPONENTS " },
      { (GLenum) 0, NULL }
   };
   static const struct token_name geometry_limits[] = {
      { GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, "GL_MAX_GEOMETRY_UNIFORM_COMPONENTS" },
      { GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, "GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS" },
      { GL_MAX_GEOMETRY_OUTPUT_VERTICES  , "GL_MAX_GEOMETRY_OUTPUT_VERTICES  " },
      { GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, "GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS" },
      { GL_MAX_GEOMETRY_INPUT_COMPONENTS , "GL_MAX_GEOMETRY_INPUT_COMPONENTS " },
      { GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, "GL_MAX_GEOMETRY_OUTPUT_COMPONENTS" },
      { (GLenum) 0, NULL }
   };

   switch (target) {
   case GL_VERTEX_SHADER:
      printf("    GL_VERTEX_SHADER_ARB:\n");
      print_shader_limit_list(vertex_limits);
      break;

   case GL_FRAGMENT_SHADER:
      printf("    GL_FRAGMENT_SHADER_ARB:\n");
      print_shader_limit_list(fragment_limits);
      break;

   case GL_GEOMETRY_SHADER:
      printf("    GL_GEOMETRY_SHADER:\n");
      print_shader_limit_list(geometry_limits);
      break;
   }
}


/**
 * Print interesting limits for vertex/fragment programs.
 */
static void
print_program_limits(GLenum target,
                     const struct ext_functions *extfuncs)
{
#if defined(GL_ARB_vertex_program) || defined(GL_ARB_fragment_program)
   struct token_name {
      GLenum token;
      const char *name;
   };
   static const struct token_name common_limits[] = {
      { GL_MAX_PROGRAM_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_TEMPORARIES_ARB, "GL_MAX_PROGRAM_TEMPORARIES_ARB" },
      { GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB, "GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB" },
      { GL_MAX_PROGRAM_PARAMETERS_ARB, "GL_MAX_PROGRAM_PARAMETERS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB, "GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB" },
      { GL_MAX_PROGRAM_ATTRIBS_ARB, "GL_MAX_PROGRAM_ATTRIBS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB, "GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB" },
      { GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB, "GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB, "GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB" },
      { GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, "GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB" },
      { GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, "GL_MAX_PROGRAM_ENV_PARAMETERS_ARB" },
      { (GLenum) 0, NULL }
   };
   static const struct token_name fragment_limits[] = {
      { GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB, "GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB, "GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB" },
      { GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB, "GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB" },
      { (GLenum) 0, NULL }
   };

   GLint max[1];
   int i;

   if (target == GL_VERTEX_PROGRAM_ARB) {
      printf("    GL_VERTEX_PROGRAM_ARB:\n");
   }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
      printf("    GL_FRAGMENT_PROGRAM_ARB:\n");
   }
   else {
      return; /* something's wrong */
   }

   for (i = 0; common_limits[i].token; i++) {
      extfuncs->GetProgramivARB(target, common_limits[i].token, max);
      if (glGetError() == GL_NO_ERROR) {
         printf("        %s = %d\n", common_limits[i].name, max[0]);
      }
   }
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      for (i = 0; fragment_limits[i].token; i++) {
         extfuncs->GetProgramivARB(target, fragment_limits[i].token, max);
         if (glGetError() == GL_NO_ERROR) {
            printf("        %s = %d\n", fragment_limits[i].name, max[0]);
         }
      }
   }
#endif /* GL_ARB_vertex_program / GL_ARB_fragment_program */
}


/**
 * Print interesting OpenGL implementation limits.
 * \param version  20, 21, 30, 31, 32, etc.
 */
void
print_limits(const char *extensions, const char *oglstring, int version,
             const struct ext_functions *extfuncs)
{
   struct token_name {
      GLuint count;
      GLenum token;
      const char *name;
      const char *extension; /* NULL or GL extension name or version string */
   };
   static const struct token_name limits[] = {
      { 1, GL_MAX_ATTRIB_STACK_DEPTH, "GL_MAX_ATTRIB_STACK_DEPTH", NULL },
      { 1, GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH", NULL },
      { 1, GL_MAX_CLIP_PLANES, "GL_MAX_CLIP_PLANES", NULL },
      { 1, GL_MAX_COLOR_MATRIX_STACK_DEPTH, "GL_MAX_COLOR_MATRIX_STACK_DEPTH", "GL_ARB_imaging" },
      { 1, GL_MAX_ELEMENTS_VERTICES, "GL_MAX_ELEMENTS_VERTICES", NULL },
      { 1, GL_MAX_ELEMENTS_INDICES, "GL_MAX_ELEMENTS_INDICES", NULL },
      { 1, GL_MAX_EVAL_ORDER, "GL_MAX_EVAL_ORDER", NULL },
      { 1, GL_MAX_LIGHTS, "GL_MAX_LIGHTS", NULL },
      { 1, GL_MAX_LIST_NESTING, "GL_MAX_LIST_NESTING", NULL },
      { 1, GL_MAX_MODELVIEW_STACK_DEPTH, "GL_MAX_MODELVIEW_STACK_DEPTH", NULL },
      { 1, GL_MAX_NAME_STACK_DEPTH, "GL_MAX_NAME_STACK_DEPTH", NULL },
      { 1, GL_MAX_PIXEL_MAP_TABLE, "GL_MAX_PIXEL_MAP_TABLE", NULL },
      { 1, GL_MAX_PROJECTION_STACK_DEPTH, "GL_MAX_PROJECTION_STACK_DEPTH", NULL },
      { 1, GL_MAX_TEXTURE_STACK_DEPTH, "GL_MAX_TEXTURE_STACK_DEPTH", NULL },
      { 1, GL_MAX_TEXTURE_SIZE, "GL_MAX_TEXTURE_SIZE", NULL },
      { 1, GL_MAX_3D_TEXTURE_SIZE, "GL_MAX_3D_TEXTURE_SIZE", NULL },
#if defined(GL_EXT_texture_array)
      { 1, GL_MAX_ARRAY_TEXTURE_LAYERS_EXT, "GL_MAX_ARRAY_TEXTURE_LAYERS", "GL_EXT_texture_array" },
#endif
      { 2, GL_MAX_VIEWPORT_DIMS, "GL_MAX_VIEWPORT_DIMS", NULL },
      { 2, GL_ALIASED_LINE_WIDTH_RANGE, "GL_ALIASED_LINE_WIDTH_RANGE", NULL },
      { 2, GL_SMOOTH_LINE_WIDTH_RANGE, "GL_SMOOTH_LINE_WIDTH_RANGE", NULL },
      { 2, GL_ALIASED_POINT_SIZE_RANGE, "GL_ALIASED_POINT_SIZE_RANGE", NULL },
      { 2, GL_SMOOTH_POINT_SIZE_RANGE, "GL_SMOOTH_POINT_SIZE_RANGE", NULL },
#if defined(GL_ARB_texture_cube_map)
      { 1, GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB", "GL_ARB_texture_cube_map" },
#endif
#if defined(GL_NV_texture_rectangle)
      { 1, GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV", "GL_NV_texture_rectangle" },
#endif
#if defined(GL_ARB_multitexture)
      { 1, GL_MAX_TEXTURE_UNITS_ARB, "GL_MAX_TEXTURE_UNITS_ARB", "GL_ARB_multitexture" },
#endif
#if defined(GL_EXT_texture_lod_bias)
      { 1, GL_MAX_TEXTURE_LOD_BIAS_EXT, "GL_MAX_TEXTURE_LOD_BIAS_EXT", "GL_EXT_texture_lod_bias" },
#endif
#if defined(GL_EXT_texture_filter_anisotropic)
      { 1, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT", "GL_EXT_texture_filter_anisotropic" },
#endif
#if defined(GL_ARB_draw_buffers)
      { 1, GL_MAX_DRAW_BUFFERS_ARB, "GL_MAX_DRAW_BUFFERS_ARB", "GL_ARB_draw_buffers" },
#endif
#if defined(GL_ARB_blend_func_extended)
      { 1, GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, "GL_MAX_DUAL_SOURCE_DRAW_BUFFERS", "GL_ARB_blend_func_extended" },
#endif
#if defined (GL_ARB_framebuffer_object)
      { 1, GL_MAX_RENDERBUFFER_SIZE, "GL_MAX_RENDERBUFFER_SIZE", "GL_ARB_framebuffer_object" },
      { 1, GL_MAX_COLOR_ATTACHMENTS, "GL_MAX_COLOR_ATTACHMENTS", "GL_ARB_framebuffer_object" },
      { 1, GL_MAX_SAMPLES, "GL_MAX_SAMPLES", "GL_ARB_framebuffer_object" },
#endif
#if defined (GL_EXT_transform_feedback)
     { 1, GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, "GL_MAX_TRANSFORM_FEEDBACK_BUFFERS", "GL_EXT_transform_feedback" },
     { 1, GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT, "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS", "GL_EXT_transform_feedback" },
     { 1, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT, "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS", "GL_EXT_transform_feedback", },
     { 1, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT, "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS", "GL_EXT_transform_feedback" },
#endif
#if defined (GL_ARB_texture_buffer_object)
      { 1, GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT, "GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT", "GL_ARB_texture_buffer_object" },
      { 1, GL_MAX_TEXTURE_BUFFER_SIZE, "GL_MAX_TEXTURE_BUFFER_SIZE", "GL_ARB_texture_buffer_object" },
#endif
#if defined (GL_ARB_texture_multisample)
      { 1, GL_MAX_COLOR_TEXTURE_SAMPLES, "GL_MAX_COLOR_TEXTURE_SAMPLES", "GL_ARB_texture_multisample" },
      { 1, GL_MAX_DEPTH_TEXTURE_SAMPLES, "GL_MAX_DEPTH_TEXTURE_SAMPLES", "GL_ARB_texture_multisample" },
      { 1, GL_MAX_INTEGER_SAMPLES, "GL_MAX_INTEGER_SAMPLES", "GL_ARB_texture_multisample" },
#endif
#if defined (GL_ARB_uniform_buffer_object)
      { 1, GL_MAX_VERTEX_UNIFORM_BLOCKS, "GL_MAX_VERTEX_UNIFORM_BLOCKS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_FRAGMENT_UNIFORM_BLOCKS, "GL_MAX_FRAGMENT_UNIFORM_BLOCKS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_GEOMETRY_UNIFORM_BLOCKS, "GL_MAX_GEOMETRY_UNIFORM_BLOCKS" , "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_COMBINED_UNIFORM_BLOCKS, "GL_MAX_COMBINED_UNIFORM_BLOCKS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_UNIFORM_BUFFER_BINDINGS, "GL_MAX_UNIFORM_BUFFER_BINDINGS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_UNIFORM_BLOCK_SIZE, "GL_MAX_UNIFORM_BLOCK_SIZE", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS", "GL_ARB_uniform_buffer_object" },
      { 1, GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, "GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT", "GL_ARB_uniform_buffer_object" },
#endif
#if defined (GL_ARB_vertex_attrib_binding)
      { 1, GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET", "GL_ARB_vertex_attrib_binding" },
      { 1, GL_MAX_VERTEX_ATTRIB_BINDINGS, "GL_MAX_VERTEX_ATTRIB_BINDINGS", "GL_ARB_vertex_attrib_binding" },
#endif
#if defined(GL_ARB_tessellation_shader)
      { 1, GL_MAX_TESS_GEN_LEVEL, "GL_MAX_TESS_GEN_LEVEL", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_PATCH_COMPONENTS, "GL_MAX_TESS_PATCH_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS, "GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS, "GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS, "GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, "GL_MAX_TESS_CONTROL_INPUT_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, "GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS, "GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS, "GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS, "GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS, "GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, "GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, "GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS", "GL_ARB_tessellation_shader" },
      { 1, GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS", "GL_ARB_tessellation_shader" },
#endif

#if defined(GL_VERSION_3_0)
      { 1, GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, "GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS", "3.0" },
      { 1, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS", "3.0" },
      { 1, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, "GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS", "3.0" },
#endif
#if defined(GL_VERSION_3_1)
      { 1, GL_MAX_TEXTURE_BUFFER_SIZE, "GL_MAX_TEXTURE_BUFFER_SIZE", "3.1" },
      { 1, GL_MAX_RECTANGLE_TEXTURE_SIZE, "GL_MAX_RECTANGLE_TEXTURE_SIZE", "3.1" },
#endif
#if defined(GL_VERSION_3_2)
      { 1, GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, "GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS", "3.2" },
      { 1, GL_MAX_GEOMETRY_UNIFORM_COMPONENTS, "GL_MAX_GEOMETRY_UNIFORM_COMPONENTS", "3.2" },
      { 1, GL_MAX_GEOMETRY_OUTPUT_VERTICES, "GL_MAX_GEOMETRY_OUTPUT_VERTICES", "3.2" },
      { 1, GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, "GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS", "3.2" },
      { 1, GL_MAX_VERTEX_OUTPUT_COMPONENTS, "GL_MAX_VERTEX_OUTPUT_COMPONENTS", "3.2" },
      { 1, GL_MAX_GEOMETRY_INPUT_COMPONENTS, "GL_MAX_GEOMETRY_INPUT_COMPONENTS", "3.2" },
      { 1, GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, "GL_MAX_GEOMETRY_OUTPUT_COMPONENTS", "3.2" },
      { 1, GL_MAX_FRAGMENT_INPUT_COMPONENTS, "GL_MAX_FRAGMENT_INPUT_COMPONENTS", "3.2" },
      { 1, GL_MAX_SERVER_WAIT_TIMEOUT, "GL_MAX_SERVER_WAIT_TIMEOUT", "3.2" },
      { 1, GL_MAX_SAMPLE_MASK_WORDS, "GL_MAX_SAMPLE_MASK_WORDS", "3.2" },
      { 1, GL_MAX_COLOR_TEXTURE_SAMPLES, "GL_MAX_COLOR_TEXTURE_SAMPLES", "3.2" },
      { 1, GL_MAX_DEPTH_TEXTURE_SAMPLES, "GL_MAX_DEPTH_TEXTURE_SAMPLES", "3.2" },
      { 1, GL_MAX_INTEGER_SAMPLES, "GL_MAX_INTEGER_SAMPLES", "3.2" },
#endif
#if defined(GL_VERSION_3_3)
      { 1, GL_MAX_DUAL_SOURCE_DRAW_BUFFERS, "GL_MAX_DUAL_SOURCE_DRAW_BUFFERS", "3.3" },
#endif
#if defined(GL_VERSION_4_0)
      { 1, GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, "GL_MAX_TRANSFORM_FEEDBACK_BUFFERS", "4.0" },
#endif
#if defined(GL_VERSION_4_1)
      { 1, GL_MAX_VERTEX_UNIFORM_VECTORS, "GL_MAX_VERTEX_UNIFORM_VECTORS", "4.1" },
      { 1, GL_MAX_VARYING_VECTORS, "GL_MAX_VARYING_VECTORS", "4.1" },
      { 1, GL_MAX_FRAGMENT_UNIFORM_VECTORS, "GL_MAX_FRAGMENT_UNIFORM_VECTORS", "4.1" },
#endif
#if defined(GL_VERSION_4_2)
      { 1, GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS, "GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS, "GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS, "GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS, "GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS, "GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS", "4.2" },
      { 1, GL_MAX_VERTEX_ATOMIC_COUNTERS, "GL_MAX_VERTEX_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS, "GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS, "GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_GEOMETRY_ATOMIC_COUNTERS, "GL_MAX_GEOMETRY_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_FRAGMENT_ATOMIC_COUNTERS, "GL_MAX_FRAGMENT_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_COMBINED_ATOMIC_COUNTERS, "GL_MAX_COMBINED_ATOMIC_COUNTERS", "4.2" },
      { 1, GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, "GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE", "4.2" },
      { 1, GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, "GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS", "4.2" },
      { 1, GL_MAX_IMAGE_UNITS, "GL_MAX_IMAGE_UNITS", "4.2" },
      { 1, GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS, "GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS", "4.2" },
      { 1, GL_MAX_IMAGE_SAMPLES, "GL_MAX_IMAGE_SAMPLES", "4.2" },
      { 1, GL_MAX_VERTEX_IMAGE_UNIFORMS , "GL_MAX_VERTEX_IMAGE_UNIFORMS", "4.2" },
      { 1, GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS, "GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS", "4.2" },
      { 1, GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS, "GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS", "4.2" },
      { 1, GL_MAX_GEOMETRY_IMAGE_UNIFORMS, "GL_MAX_GEOMETRY_IMAGE_UNIFORMS", "4.2" },
      { 1, GL_MAX_FRAGMENT_IMAGE_UNIFORMS, "GL_MAX_FRAGMENT_IMAGE_UNIFORMS", "4.2" },
      { 1, GL_MAX_COMBINED_IMAGE_UNIFORMS, "GL_MAX_COMBINED_IMAGE_UNIFORMS", "4.2" },
#endif
#if defined(GL_VERSION_4_3)
      { 1, GL_MAX_ELEMENT_INDEX, "GL_MAX_ELEMENT_INDEX", "4.3" },
      { 1, GL_MAX_COMPUTE_UNIFORM_BLOCKS, "GL_MAX_COMPUTE_UNIFORM_BLOCKS", "4.3" },
      { 1, GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, "GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS", "4.3" },
      { 1, GL_MAX_COMPUTE_IMAGE_UNIFORMS, "GL_MAX_COMPUTE_IMAGE_UNIFORMS", "4.3" },
      { 1, GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE", "4.3" },
      { 1, GL_MAX_COMPUTE_UNIFORM_COMPONENTS, "GL_MAX_COMPUTE_UNIFORM_COMPONENTS", "4.3" },
      { 1, GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, "GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS", "4.3" },
      { 1, GL_MAX_COMPUTE_ATOMIC_COUNTERS, "GL_MAX_COMPUTE_ATOMIC_COUNTERS", "4.3" },
      { 1, GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, "GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS", "4.3" },
      { 1, GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS", "4.3" },
      { 1, GL_MAX_COMPUTE_WORK_GROUP_COUNT, "GL_MAX_COMPUTE_WORK_GROUP_COUNT", "4.3" },
      { 1, GL_MAX_COMPUTE_WORK_GROUP_SIZE, "GL_MAX_COMPUTE_WORK_GROUP_SIZE", "4.3" },
      { 1, GL_MAX_DEBUG_MESSAGE_LENGTH, "GL_MAX_DEBUG_MESSAGE_LENGTH", "4.3" },
      { 1, GL_MAX_DEBUG_LOGGED_MESSAGES, "GL_MAX_DEBUG_LOGGED_MESSAGES", "4.3" },
      { 1, GL_MAX_DEBUG_GROUP_STACK_DEPTH, "GL_MAX_DEBUG_GROUP_STACK_DEPTH", "4.3" },
      { 1, GL_MAX_LABEL_LENGTH, "GL_MAX_LABEL_LENGTH", "4.3" },
      { 1, GL_MAX_UNIFORM_LOCATIONS, "GL_MAX_UNIFORM_LOCATIONS", "4.3" },
      { 1, GL_MAX_FRAMEBUFFER_WIDTH, "GL_MAX_FRAMEBUFFER_WIDTH", "4.3" },
      { 1, GL_MAX_FRAMEBUFFER_HEIGHT, "GL_MAX_FRAMEBUFFER_HEIGHT", "4.3" },
      { 1, GL_MAX_FRAMEBUFFER_LAYERS, "GL_MAX_FRAMEBUFFER_LAYERS", "4.3" },
      { 1, GL_MAX_FRAMEBUFFER_SAMPLES, "GL_MAX_FRAMEBUFFER_SAMPLES", "4.3" },
      { 1, GL_MAX_WIDTH, "GL_MAX_WIDTH", "4.3" },
      { 1, GL_MAX_HEIGHT, "GL_MAX_HEIGHT", "4.3" },
      { 1, GL_MAX_DEPTH, "GL_MAX_DEPTH", "4.3" },
      { 1, GL_MAX_LAYERS, "GL_MAX_LAYERS", "4.3" },
      { 1, GL_MAX_COMBINED_DIMENSIONS, "GL_MAX_COMBINED_DIMENSIONS", "4.3" },
      { 1, GL_MAX_NAME_LENGTH, "GL_MAX_NAME_LENGTH", "4.3" },
      { 1, GL_MAX_NUM_ACTIVE_VARIABLES, "GL_MAX_NUM_ACTIVE_VARIABLES", "4.3" },
      { 1, GL_MAX_NUM_COMPATIBLE_SUBROUTINES, "GL_MAX_NUM_COMPATIBLE_SUBROUTINES", "4.3" },
      { 1, GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, "GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS, "GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS, "GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS, "GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, "GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, "GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS", "4.3" },
      { 1, GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, "GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS", "4.3" },
      { 1, GL_MAX_SHADER_STORAGE_BLOCK_SIZE, "GL_MAX_SHADER_STORAGE_BLOCK_SIZE", "4.3" },
      { 1, GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, "GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES", "4.3" },
      { 1, GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET", "4.3" },
      { 1, GL_MAX_VERTEX_ATTRIB_BINDINGS, "GL_MAX_VERTEX_ATTRIB_BINDINGS", "4.3" },
#endif
#if defined(GL_VERSION_4_4)
      { 1, GL_MAX_VERTEX_ATTRIB_STRIDE, "GL_MAX_VERTEX_ATTRIB_STRIDE", "4.4" },
#endif
#if defined(GL_VERSION_4_5)
      { 1, GL_MAX_CULL_DISTANCES, "GL_MAX_CULL_DISTANCES", "4.5" },
      { 1, GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES, "GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES", "4.5" },
#endif
#if defined(GL_VERSION_4_6)
      { 1, GL_MAX_TEXTURE_MAX_ANISOTROPY, "GL_MAX_TEXTURE_MAX_ANISOTROPY", "4.6" },
#endif
#if defined(GL_ARB_transform_feedback3)
      { 1, GL_MAX_TRANSFORM_FEEDBACK_BUFFERS, "GL_MAX_TRANSFORM_FEEDBACK_BUFFERS", "GL_ARB_transform_feedback3" },
      { 1, GL_MAX_VERTEX_STREAMS, "GL_MAX_VERTEX_STREAMS", "GL_ARB_transform_feedback3" },
#endif
      { 0, (GLenum) 0, NULL, NULL }
   };
   GLint i, max[2];
   const char *prev_ext = "none";

   printf("%s limits:\n", oglstring);
   for (i = 0; limits[i].count; i++) {
      if (!limits[i].extension ||
          version_supported(limits[i].extension, version) ||
          extension_supported(limits[i].extension, extensions)) {
         glGetIntegerv(limits[i].token, max);
         if (glGetError() == GL_NO_ERROR) {
            if (limits[i].extension && strcmp(limits[i].extension, prev_ext) != 0) {
               printf("  %s:\n", limits[i].extension);
               prev_ext = limits[i].extension;
            }
            if (limits[i].count == 1) {
               printf("    %s = %d\n", limits[i].name, max[0]);
            }
            else {
               assert(limits[i].count == 2);
               printf("    %s = %d, %d\n", limits[i].name, max[0], max[1]);
            }
         }
      }
   }

   /* these don't fit into the above mechanism, unfortunately */
   if (extension_supported("GL_ARB_imaging", extensions)) {
      extfuncs->GetConvolutionParameteriv(GL_CONVOLUTION_2D,
                                          GL_MAX_CONVOLUTION_WIDTH, max);
      extfuncs->GetConvolutionParameteriv(GL_CONVOLUTION_2D,
                                          GL_MAX_CONVOLUTION_HEIGHT, max+1);
      printf("  GL_ARB_imaging:\n");
      printf("    GL_MAX_CONVOLUTION_WIDTH/HEIGHT = %d, %d\n", max[0], max[1]);
   }

   if (extension_supported("GL_ARB_texture_compression", extensions)) {
      GLint n;
      GLint *formats;
      printf("  GL_ARB_texture_compression:\n");
      glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &n);
      printf("    GL_NUM_COMPRESSED_TEXTURE_FORMATS = %d\n", n);
      formats = (GLint *) malloc(n * sizeof(GLint));
      glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formats);
      for (i = 0; i < n; i++) {
         printf("      %s\n", enum_name(formats[i]));
      }
      free(formats);
   }

#if defined(GL_VERSION_4_3)
   if (version >= 43) {
      GLint n = 0;
      printf("  4.3:\n");
      glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &n);
      printf("    GL_NUM_SHADING_LANGUAGE_VERSIONS = %d\n", n);
      for (i = 0; i < n; i++) {
         printf("      %s\n", (const char *)
                extfuncs->GetStringi(GL_SHADING_LANGUAGE_VERSION, i));
      }
   }
#endif

#if defined(GL_ARB_vertex_program)
   if (extension_supported("GL_ARB_vertex_program", extensions)) {
      print_program_limits(GL_VERTEX_PROGRAM_ARB, extfuncs);
   }
#endif
#if defined(GL_ARB_fragment_program)
   if (extension_supported("GL_ARB_fragment_program", extensions)) {
      print_program_limits(GL_FRAGMENT_PROGRAM_ARB, extfuncs);
   }
#endif
   if (extension_supported("GL_ARB_vertex_shader", extensions)) {
      print_shader_limits(GL_VERTEX_SHADER_ARB);
   }
   if (extension_supported("GL_ARB_fragment_shader", extensions)) {
      print_shader_limits(GL_FRAGMENT_SHADER_ARB);
   }
   if (version >= 32) {
      print_shader_limits(GL_GEOMETRY_SHADER);
   }
}



/**
 * Return string representation for bits in a bitmask.
 */
const char *
bitmask_to_string(const struct bit_info bits[], int numBits, int mask)
{
   static char buffer[256], *p;
   int i;

   strcpy(buffer, "(none)");
   p = buffer;
   for (i = 0; i < numBits; i++) {
      if (mask & bits[i].bit) {
         if (p > buffer)
            *p++ = ',';
         strcpy(p, bits[i].name);
         p += strlen(bits[i].name);
      }
   }

   return buffer;
}

/**
 * Return string representation for the bitmask returned by
 * GL_CONTEXT_PROFILE_MASK (OpenGL 3.2 or later).
 */
#ifdef GL_VERSION_3_2
const char *
profile_mask_string(int mask)
{
   static const struct bit_info bits[] = {
#ifdef GL_CONTEXT_CORE_PROFILE_BIT
      { GL_CONTEXT_CORE_PROFILE_BIT, "core profile"},
#endif
#ifdef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
      { GL_CONTEXT_COMPATIBILITY_PROFILE_BIT, "compatibility profile" }
#endif
   };

   return bitmask_to_string(bits, ELEMENTS(bits), mask);
}
#endif


/**
 * Return string representation for the bitmask returned by
 * GL_CONTEXT_FLAGS (OpenGL 3.0 or later).
 */
#ifdef GL_VERSION_3_0
const char *
context_flags_string(int mask)
{
   static const struct bit_info bits[] = {
      { GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT, "forward-compatible" },
      { GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB, "robust-access" },
      { GL_CONTEXT_FLAG_DEBUG_BIT, "debug" },
      { GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR, "no-error" },
   };

   return bitmask_to_string(bits, ELEMENTS(bits), mask);
}
#endif


static void
usage(void)
{
#ifdef _WIN32
   printf("Usage: wglinfo [-v] [-t] [-h] [-b] [-l] [-s]\n");
#else
   printf("Usage: glxinfo [-v] [-t] [-h] [-b] [-l] [-s] [-i] [-c] [-display <dname>]\n");
   printf("\t-display <dname>: Print GLX visuals on specified server.\n");
   printf("\t-i: Force an indirect rendering context.\n");
#endif
   printf("\t-B: brief output, print only the basics.\n");
   printf("\t-v: Print visuals info in verbose form.\n");
   printf("\t-t: Print verbose visual information table.\n");
   printf("\t-h: This information.\n");
   printf("\t-b: Find the 'best' visual and print its number.\n");
   printf("\t-l: Print interesting OpenGL limits.\n");
   printf("\t-s: Print a single extension per line.\n");
   printf("\t-c: Print table of GLXFBConfigs instead of X Visuals\n");
}

void
parse_args(int argc, char *argv[], struct options *options)
{
   int i;

   options->mode = Normal;
   options->findBest = GL_FALSE;
   options->limits = GL_FALSE;
   options->singleLine = GL_FALSE;
   options->displayName = NULL;
   options->allowDirect = GL_TRUE;
   options->fbConfigs = GL_FALSE;

   for (i = 1; i < argc; i++) {
#ifndef _WIN32
      if (strcmp(argv[i], "-display") == 0 && i + 1 < argc) {
         options->displayName = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "-i") == 0) {
         options->allowDirect = GL_FALSE;
      }
      else
#endif
      if (strcmp(argv[i], "-t") == 0) {
         options->mode = Wide;
      }
      else if (strcmp(argv[i], "-v") == 0) {
         options->mode = Verbose;
      }
      else if (strcmp(argv[i], "-B") == 0) {
         options->mode = Brief;
      }
      else if (strcmp(argv[i], "-b") == 0) {
         options->findBest = GL_TRUE;
      }
      else if (strcmp(argv[i], "-l") == 0) {
         options->limits = GL_TRUE;
      }
      else if (strcmp(argv[i], "-h") == 0) {
         usage();
         exit(0);
      }
      else if (strcmp(argv[i], "-s") == 0) {
         options->singleLine = GL_TRUE;
      }
      else if (strcmp(argv[i], "-c") == 0) {
         options->fbConfigs = GL_TRUE;
      }
      else {
         printf("Unknown option `%s'\n", argv[i]);
         usage();
         exit(0);
      }
   }
}

static void
query_ATI_meminfo(void)
{
#ifdef GL_ATI_meminfo
    int i[4];

    printf("Memory info (GL_ATI_meminfo):\n");

    glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, i);
    printf("    VBO free memory - total: %u MB, largest block: %u MB\n",
           i[0] / 1024, i[1] / 1024);
    printf("    VBO free aux. memory - total: %u MB, largest block: %u MB\n",
           i[2] / 1024, i[3] / 1024);

    glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, i);
    printf("    Texture free memory - total: %u MB, largest block: %u MB\n",
           i[0] / 1024, i[1] / 1024);
    printf("    Texture free aux. memory - total: %u MB, largest block: %u MB\n",
           i[2] / 1024, i[3] / 1024);

    glGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, i);
    printf("    Renderbuffer free memory - total: %u MB, largest block: %u MB\n",
           i[0] / 1024, i[1] / 1024);
    printf("    Renderbuffer free aux. memory - total: %u MB, largest block: %u MB\n",
           i[2] / 1024, i[3] / 1024);
#endif
}

static void
query_NVX_gpu_memory_info(void)
{
#ifdef GL_NVX_gpu_memory_info
    int i;

    printf("Memory info (GL_NVX_gpu_memory_info):\n");

    glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &i);
    printf("    Dedicated video memory: %u MB\n", i / 1024);

    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &i);
    printf("    Total available memory: %u MB\n", i / 1024);

    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &i);
    printf("    Currently available dedicated video memory: %u MB\n", i / 1024);
#endif
}

void
print_gpu_memory_info(const char *glExtensions)
{
   if (strstr(glExtensions, "GL_ATI_meminfo"))
      query_ATI_meminfo();
   if (strstr(glExtensions, "GL_NVX_gpu_memory_info"))
      query_NVX_gpu_memory_info();
}
