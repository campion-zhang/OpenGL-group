/*
 * Copyright © 2017 Red Hat
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "nir/nir.h"
#include "nir/nir_xfb_info.h"
#include "radv_private.h"
#include "radv_shader.h"

#include "ac_nir.h"

static void
mark_sampler_desc(const nir_variable *var, struct radv_shader_info *info)
{
   info->desc_set_used_mask |= (1u << var->data.descriptor_set);
}

static void
gather_intrinsic_load_input_info(const nir_shader *nir, const nir_intrinsic_instr *instr,
                                 struct radv_shader_info *info)
{
   switch (nir->info.stage) {
   case MESA_SHADER_VERTEX: {
      unsigned idx = nir_intrinsic_io_semantics(instr).location;
      unsigned component = nir_intrinsic_component(instr);
      unsigned mask = nir_ssa_def_components_read(&instr->dest.ssa);

      info->vs.input_usage_mask[idx] |= mask << component;
      break;
   }
   default:
      break;
   }
}

static void
set_writes_memory(const nir_shader *nir, struct radv_shader_info *info)
{
   if (nir->info.stage == MESA_SHADER_FRAGMENT)
      info->ps.writes_memory = true;
}

static void
gather_intrinsic_store_output_info(const nir_shader *nir, const nir_intrinsic_instr *instr,
                                   struct radv_shader_info *info)
{
   unsigned idx = nir_intrinsic_base(instr);
   unsigned num_slots = nir_intrinsic_io_semantics(instr).num_slots;
   unsigned component = nir_intrinsic_component(instr);
   unsigned write_mask = nir_intrinsic_write_mask(instr);
   uint8_t *output_usage_mask = NULL;

   if (instr->src[0].ssa->bit_size == 64)
      write_mask = util_widen_mask(write_mask, 2);

   switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
      output_usage_mask = info->vs.output_usage_mask;
      break;
   case MESA_SHADER_TESS_EVAL:
      output_usage_mask = info->tes.output_usage_mask;
      break;
   case MESA_SHADER_GEOMETRY:
      output_usage_mask = info->gs.output_usage_mask;
      break;
   default:
      break;
   }

   if (output_usage_mask) {
      for (unsigned i = 0; i < num_slots; i++) {
         output_usage_mask[idx + i] |= ((write_mask >> (i * 4)) & 0xf) << component;
      }
   }
}

static void
gather_push_constant_info(const nir_shader *nir, const nir_intrinsic_instr *instr,
                          struct radv_shader_info *info)
{
   info->loads_push_constants = true;

   if (nir_src_is_const(instr->src[0]) && instr->dest.ssa.bit_size >= 32) {
      uint32_t start = (nir_intrinsic_base(instr) + nir_src_as_uint(instr->src[0])) / 4u;
      uint32_t size = instr->num_components * (instr->dest.ssa.bit_size / 32u);

      if (start + size <= (MAX_PUSH_CONSTANTS_SIZE / 4u)) {
         info->inline_push_constant_mask |= u_bit_consecutive64(start, size);
         return;
      }
   }

   info->can_inline_all_push_constants = false;
}

static void
gather_intrinsic_info(const nir_shader *nir, const nir_intrinsic_instr *instr,
                      struct radv_shader_info *info)
{
   switch (instr->intrinsic) {
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset: {
      enum glsl_interp_mode mode = nir_intrinsic_interp_mode(instr);
      switch (mode) {
      case INTERP_MODE_SMOOTH:
      case INTERP_MODE_NONE:
         if (instr->intrinsic == nir_intrinsic_load_barycentric_pixel ||
             instr->intrinsic == nir_intrinsic_load_barycentric_at_sample ||
             instr->intrinsic == nir_intrinsic_load_barycentric_at_offset)
            info->ps.reads_persp_center = true;
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_centroid)
            info->ps.reads_persp_centroid = true;
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_sample)
            info->ps.reads_persp_sample = true;
         break;
      case INTERP_MODE_NOPERSPECTIVE:
         if (instr->intrinsic == nir_intrinsic_load_barycentric_pixel ||
             instr->intrinsic == nir_intrinsic_load_barycentric_at_sample ||
             instr->intrinsic == nir_intrinsic_load_barycentric_at_offset)
            info->ps.reads_linear_center = true;
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_centroid)
            info->ps.reads_linear_centroid = true;
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_sample)
            info->ps.reads_linear_sample = true;
         break;
      default:
         break;
      }
      if (instr->intrinsic == nir_intrinsic_load_barycentric_at_sample)
         info->ps.needs_sample_positions = true;
      break;
   }
   case nir_intrinsic_load_barycentric_model:
      info->ps.reads_barycentric_model = true;
      break;
   case nir_intrinsic_load_draw_id:
      info->vs.needs_draw_id = true;
      break;
   case nir_intrinsic_load_base_instance:
      info->vs.needs_base_instance = true;
      break;
   case nir_intrinsic_load_instance_id:
      info->vs.needs_instance_id = true;
      break;
   case nir_intrinsic_load_num_workgroups:
      info->cs.uses_grid_size = true;
      break;
   case nir_intrinsic_load_ray_launch_size:
      info->cs.uses_ray_launch_size = true;
      break;
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_workgroup_id: {
      unsigned mask = nir_ssa_def_components_read(&instr->dest.ssa);
      while (mask) {
         unsigned i = u_bit_scan(&mask);

         if (instr->intrinsic == nir_intrinsic_load_workgroup_id)
            info->cs.uses_block_id[i] = true;
         else
            info->cs.uses_thread_id[i] = true;
      }
      break;
   }
   case nir_intrinsic_load_local_invocation_index:
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_num_subgroups:
      info->cs.uses_local_invocation_idx = true;
      break;
   case nir_intrinsic_load_sample_mask_in:
      info->ps.reads_sample_mask_in = true;
      break;
   case nir_intrinsic_load_sample_id:
      info->ps.reads_sample_id = true;
      break;
   case nir_intrinsic_load_frag_shading_rate:
      info->ps.reads_frag_shading_rate = true;
      break;
   case nir_intrinsic_load_front_face:
      info->ps.reads_front_face = true;
      break;
   case nir_intrinsic_load_frag_coord:
      info->ps.reads_frag_coord_mask = nir_ssa_def_components_read(&instr->dest.ssa);
      break;
   case nir_intrinsic_load_sample_pos:
      info->ps.reads_sample_pos_mask = nir_ssa_def_components_read(&instr->dest.ssa);
      break;
   case nir_intrinsic_load_view_index:
      info->uses_view_index = true;
      break;
   case nir_intrinsic_load_invocation_id:
      info->uses_invocation_id = true;
      break;
   case nir_intrinsic_load_primitive_id:
      info->uses_prim_id = true;
      break;
   case nir_intrinsic_load_push_constant:
      gather_push_constant_info(nir, instr, info);
      break;
   case nir_intrinsic_vulkan_resource_index:
      info->desc_set_used_mask |= (1u << nir_intrinsic_desc_set(instr));
      break;
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic_add:
   case nir_intrinsic_image_deref_atomic_imin:
   case nir_intrinsic_image_deref_atomic_umin:
   case nir_intrinsic_image_deref_atomic_imax:
   case nir_intrinsic_image_deref_atomic_umax:
   case nir_intrinsic_image_deref_atomic_and:
   case nir_intrinsic_image_deref_atomic_or:
   case nir_intrinsic_image_deref_atomic_xor:
   case nir_intrinsic_image_deref_atomic_exchange:
   case nir_intrinsic_image_deref_atomic_comp_swap:
   case nir_intrinsic_image_deref_atomic_fmin:
   case nir_intrinsic_image_deref_atomic_fmax:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples: {
      nir_variable *var =
         nir_deref_instr_get_variable(nir_instr_as_deref(instr->src[0].ssa->parent_instr));
      mark_sampler_desc(var, info);

      if (instr->intrinsic == nir_intrinsic_image_deref_store ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_add ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_imin ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_umin ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_imax ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_umax ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_and ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_or ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_xor ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_exchange ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_comp_swap ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_fmin ||
          instr->intrinsic == nir_intrinsic_image_deref_atomic_fmax) {
         set_writes_memory(nir, info);
      }
      break;
   }
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_ssbo_atomic_add:
   case nir_intrinsic_ssbo_atomic_imin:
   case nir_intrinsic_ssbo_atomic_umin:
   case nir_intrinsic_ssbo_atomic_imax:
   case nir_intrinsic_ssbo_atomic_umax:
   case nir_intrinsic_ssbo_atomic_and:
   case nir_intrinsic_ssbo_atomic_or:
   case nir_intrinsic_ssbo_atomic_xor:
   case nir_intrinsic_ssbo_atomic_exchange:
   case nir_intrinsic_ssbo_atomic_comp_swap:
   case nir_intrinsic_ssbo_atomic_fmin:
   case nir_intrinsic_ssbo_atomic_fmax:
   case nir_intrinsic_store_global:
   case nir_intrinsic_global_atomic_add:
   case nir_intrinsic_global_atomic_imin:
   case nir_intrinsic_global_atomic_umin:
   case nir_intrinsic_global_atomic_imax:
   case nir_intrinsic_global_atomic_umax:
   case nir_intrinsic_global_atomic_and:
   case nir_intrinsic_global_atomic_or:
   case nir_intrinsic_global_atomic_xor:
   case nir_intrinsic_global_atomic_exchange:
   case nir_intrinsic_global_atomic_comp_swap:
   case nir_intrinsic_global_atomic_fmin:
   case nir_intrinsic_global_atomic_fmax:
      set_writes_memory(nir, info);
      break;
   case nir_intrinsic_load_input:
      gather_intrinsic_load_input_info(nir, instr, info);
      break;
   case nir_intrinsic_store_output:
      gather_intrinsic_store_output_info(nir, instr, info);
      break;
   case nir_intrinsic_load_sbt_amd:
      info->cs.uses_sbt = true;
      break;
   case nir_intrinsic_load_force_vrs_rates_amd:
      info->force_vrs_per_vertex = true;
      break;
   default:
      break;
   }
}

static void
gather_tex_info(const nir_shader *nir, const nir_tex_instr *instr, struct radv_shader_info *info)
{
   for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
      case nir_tex_src_texture_deref:
         mark_sampler_desc(nir_deref_instr_get_variable(nir_src_as_deref(instr->src[i].src)), info);
         break;
      case nir_tex_src_sampler_deref:
         mark_sampler_desc(nir_deref_instr_get_variable(nir_src_as_deref(instr->src[i].src)), info);
         break;
      default:
         break;
      }
   }
}

static void
gather_info_block(const nir_shader *nir, const nir_block *block, struct radv_shader_info *info)
{
   nir_foreach_instr (instr, block) {
      switch (instr->type) {
      case nir_instr_type_intrinsic:
         gather_intrinsic_info(nir, nir_instr_as_intrinsic(instr), info);
         break;
      case nir_instr_type_tex:
         gather_tex_info(nir, nir_instr_as_tex(instr), info);
         break;
      default:
         break;
      }
   }
}

static void
gather_info_input_decl_vs(const nir_shader *nir, const nir_variable *var,
                          const struct radv_pipeline_key *key, struct radv_shader_info *info)
{
   unsigned attrib_count = glsl_count_attribute_slots(var->type, true);

   for (unsigned i = 0; i < attrib_count; ++i) {
      unsigned attrib_index = var->data.location + i - VERT_ATTRIB_GENERIC0;

      if (key->vs.instance_rate_inputs & (1u << attrib_index)) {
         info->vs.needs_instance_id = true;
         info->vs.needs_base_instance = true;
      }

      if (info->vs.use_per_attribute_vb_descs)
         info->vs.vb_desc_usage_mask |= 1u << attrib_index;
      else
         info->vs.vb_desc_usage_mask |= 1u << key->vs.vertex_attribute_bindings[attrib_index];
   }
}

static void
mark_16bit_ps_input(struct radv_shader_info *info, const struct glsl_type *type, int location)
{
   if (glsl_type_is_scalar(type) || glsl_type_is_vector(type) || glsl_type_is_matrix(type)) {
      unsigned attrib_count = glsl_count_attribute_slots(type, false);
      if (glsl_type_is_16bit(type)) {
         info->ps.float16_shaded_mask |= ((1ull << attrib_count) - 1) << location;
      }
   } else if (glsl_type_is_array(type)) {
      unsigned stride = glsl_count_attribute_slots(glsl_get_array_element(type), false);
      for (unsigned i = 0; i < glsl_get_length(type); ++i) {
         mark_16bit_ps_input(info, glsl_get_array_element(type), location + i * stride);
      }
   } else {
      assert(glsl_type_is_struct_or_ifc(type));
      for (unsigned i = 0; i < glsl_get_length(type); i++) {
         mark_16bit_ps_input(info, glsl_get_struct_field(type, i), location);
         location += glsl_count_attribute_slots(glsl_get_struct_field(type, i), false);
      }
   }
}
static void
gather_info_input_decl_ps(const nir_shader *nir, const nir_variable *var,
                          struct radv_shader_info *info)
{
   unsigned attrib_count = glsl_count_attribute_slots(var->type, false);
   int idx = var->data.location;

   switch (idx) {
   case VARYING_SLOT_PNTC:
      info->ps.has_pcoord = true;
      break;
   case VARYING_SLOT_PRIMITIVE_ID:
      info->ps.prim_id_input = true;
      break;
   case VARYING_SLOT_LAYER:
      info->ps.layer_input = true;
      break;
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
      info->ps.num_input_clips_culls += attrib_count;
      break;
   case VARYING_SLOT_VIEWPORT:
      info->ps.viewport_index_input = true;
      break;
   default:
      break;
   }

   if (var->data.compact) {
      unsigned component_count = var->data.location_frac + glsl_get_length(var->type);
      attrib_count = (component_count + 3) / 4;
   } else {
      mark_16bit_ps_input(info, var->type, var->data.driver_location);
   }

   uint64_t mask = ((1ull << attrib_count) - 1);

   if (!var->data.per_primitive) {
      if (var->data.interpolation == INTERP_MODE_FLAT)
         info->ps.flat_shaded_mask |= mask << var->data.driver_location;
      else if (var->data.interpolation == INTERP_MODE_EXPLICIT)
         info->ps.explicit_shaded_mask |= mask << var->data.driver_location;
   }

   if (var->data.location >= VARYING_SLOT_VAR0) {
      if (var->data.per_primitive)
         info->ps.input_per_primitive_mask |= mask << (var->data.location - VARYING_SLOT_VAR0);
      else
         info->ps.input_mask |= mask << (var->data.location - VARYING_SLOT_VAR0);
   }
}

static void
gather_info_input_decl(const nir_shader *nir, const nir_variable *var,
                       const struct radv_pipeline_key *key, struct radv_shader_info *info)
{
   switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
      gather_info_input_decl_vs(nir, var, key, info);
      break;
   case MESA_SHADER_FRAGMENT:
      gather_info_input_decl_ps(nir, var, info);
      break;
   default:
      break;
   }
}

static void
gather_info_output_decl_ps(const nir_shader *nir, const nir_variable *var,
                           struct radv_shader_info *info)
{
   int idx = var->data.location;

   switch (idx) {
   case FRAG_RESULT_DEPTH:
      info->ps.writes_z = true;
      break;
   case FRAG_RESULT_STENCIL:
      info->ps.writes_stencil = true;
      break;
   case FRAG_RESULT_SAMPLE_MASK:
      info->ps.writes_sample_mask = true;
      break;
   default:
      break;
   }
}

static void
gather_info_output_decl_gs(const nir_shader *nir, const nir_variable *var,
                           struct radv_shader_info *info)
{
   unsigned num_components = glsl_get_component_slots(var->type);
   unsigned stream = var->data.stream;
   unsigned idx = var->data.location;

   assert(stream < 4);

   info->gs.max_stream = MAX2(info->gs.max_stream, stream);
   info->gs.num_stream_output_components[stream] += num_components;
   info->gs.output_streams[idx] = stream;
}

static struct radv_vs_output_info *
get_vs_output_info(const nir_shader *nir, struct radv_shader_info *info)
{

   switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
      if (!info->vs.as_ls && !info->vs.as_es)
         return &info->vs.outinfo;
      break;
   case MESA_SHADER_GEOMETRY:
      return &info->vs.outinfo;
      break;
   case MESA_SHADER_TESS_EVAL:
      if (!info->tes.as_es)
         return &info->tes.outinfo;
      break;
   case MESA_SHADER_MESH:
      return &info->ms.outinfo;
   default:
      break;
   }

   return NULL;
}

static void
gather_info_output_decl(const nir_shader *nir, const nir_variable *var,
                        struct radv_shader_info *info)
{
   struct radv_vs_output_info *vs_info = get_vs_output_info(nir, info);

   switch (nir->info.stage) {
   case MESA_SHADER_FRAGMENT:
      gather_info_output_decl_ps(nir, var, info);
      break;
   case MESA_SHADER_VERTEX:
      break;
   case MESA_SHADER_GEOMETRY:
      gather_info_output_decl_gs(nir, var, info);
      break;
   case MESA_SHADER_TESS_EVAL:
      break;
   default:
      break;
   }

   if (vs_info) {
      switch (var->data.location) {
      case VARYING_SLOT_CLIP_DIST0:
         vs_info->clip_dist_mask = (1 << nir->info.clip_distance_array_size) - 1;
         vs_info->cull_dist_mask = (1 << nir->info.cull_distance_array_size) - 1;
         vs_info->cull_dist_mask <<= nir->info.clip_distance_array_size;
         break;
      case VARYING_SLOT_PSIZ:
         vs_info->writes_pointsize = true;
         break;
      case VARYING_SLOT_VIEWPORT:
         if (var->data.per_primitive)
            vs_info->writes_viewport_index_per_primitive = true;
         else
            vs_info->writes_viewport_index = true;
         break;
      case VARYING_SLOT_LAYER:
         if (var->data.per_primitive)
            vs_info->writes_layer_per_primitive = true;
         else
            vs_info->writes_layer = true;
         break;
      case VARYING_SLOT_PRIMITIVE_SHADING_RATE:
         if (var->data.per_primitive)
            vs_info->writes_primitive_shading_rate_per_primitive = true;
         else
            vs_info->writes_primitive_shading_rate = true;
         break;
      default:
         break;
      }
   }
}

static void
gather_xfb_info(const nir_shader *nir, struct radv_shader_info *info)
{
   nir_xfb_info *xfb = nir_gather_xfb_info(nir, NULL);
   struct radv_streamout_info *so = &info->so;

   if (!xfb)
      return;

   assert(xfb->output_count < MAX_SO_OUTPUTS);
   so->num_outputs = xfb->output_count;

   for (unsigned i = 0; i < xfb->output_count; i++) {
      struct radv_stream_output *output = &so->outputs[i];

      output->buffer = xfb->outputs[i].buffer;
      output->stream = xfb->buffer_to_stream[xfb->outputs[i].buffer];
      output->offset = xfb->outputs[i].offset;
      output->location = xfb->outputs[i].location;
      output->component_mask = xfb->outputs[i].component_mask;

      so->enabled_stream_buffers_mask |= (1 << output->buffer) << (output->stream * 4);
   }

   for (unsigned i = 0; i < NIR_MAX_XFB_BUFFERS; i++) {
      so->strides[i] = xfb->buffers[i].stride / 4;
   }

   ralloc_free(xfb);
}

static void
assign_outinfo_param(struct radv_vs_output_info *outinfo, gl_varying_slot idx,
                     unsigned *total_param_exports)
{
   if (outinfo->vs_output_param_offset[idx] == AC_EXP_PARAM_UNDEFINED)
      outinfo->vs_output_param_offset[idx] = (*total_param_exports)++;
}

static void
assign_outinfo_params(struct radv_vs_output_info *outinfo, uint64_t mask,
                      unsigned *total_param_exports)
{
   u_foreach_bit64(idx, mask) {
      if (idx >= VARYING_SLOT_VAR0 || idx == VARYING_SLOT_LAYER ||
          idx == VARYING_SLOT_PRIMITIVE_ID || idx == VARYING_SLOT_VIEWPORT ||
          ((idx == VARYING_SLOT_CLIP_DIST0 || idx == VARYING_SLOT_CLIP_DIST1) &&
           outinfo->export_clip_dists))
         assign_outinfo_param(outinfo, idx, total_param_exports);
   }
}

void
radv_nir_shader_info_init(struct radv_shader_info *info)
{
   /* Assume that shaders can inline all push constants by default. */
   info->can_inline_all_push_constants = true;
}

void
radv_nir_shader_info_pass(struct radv_device *device, const struct nir_shader *nir,
                          const struct radv_pipeline_layout *layout,
                          const struct radv_pipeline_key *pipeline_key,
                          struct radv_shader_info *info)
{
   struct nir_function *func = (struct nir_function *)exec_list_get_head_const(&nir->functions);

   if (layout && layout->dynamic_offset_count &&
       (layout->dynamic_shader_stages & mesa_to_vk_shader_stage(nir->info.stage))) {
      info->loads_push_constants = true;
      info->loads_dynamic_offsets = true;
   }

   if (nir->info.stage == MESA_SHADER_VERTEX) {
      if (pipeline_key->vs.dynamic_input_state && nir->info.inputs_read) {
         info->vs.has_prolog = true;
         info->vs.dynamic_inputs = true;
      }

      /* Use per-attribute vertex descriptors to prevent faults and
       * for correct bounds checking.
       */
      info->vs.use_per_attribute_vb_descs = device->robust_buffer_access || info->vs.dynamic_inputs;
   }

   /* We have to ensure consistent input register assignments between the main shader and the
    * prolog. */
   info->vs.needs_instance_id |= info->vs.has_prolog;
   info->vs.needs_base_instance |= info->vs.has_prolog;
   info->vs.needs_draw_id |= info->vs.has_prolog;

   nir_foreach_shader_in_variable (variable, nir)
      gather_info_input_decl(nir, variable, pipeline_key, info);

   nir_foreach_block (block, func->impl) {
      gather_info_block(nir, block, info);
   }

   nir_foreach_shader_out_variable(variable, nir) gather_info_output_decl(nir, variable, info);

   if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL ||
       nir->info.stage == MESA_SHADER_GEOMETRY)
      gather_xfb_info(nir, info);

   struct radv_vs_output_info *outinfo = get_vs_output_info(nir, info);
   if (outinfo) {
      int pos_written = 0x1;

      if (outinfo->writes_pointsize || outinfo->writes_viewport_index || outinfo->writes_layer ||
          outinfo->writes_primitive_shading_rate)
         pos_written |= 1 << 1;

      unsigned num_clip_distances = util_bitcount(outinfo->clip_dist_mask);
      unsigned num_cull_distances = util_bitcount(outinfo->cull_dist_mask);

      if (num_clip_distances + num_cull_distances > 0)
         pos_written |= 1 << 2;
      if (num_clip_distances + num_cull_distances > 4)
         pos_written |= 1 << 3;

      outinfo->pos_exports = util_bitcount(pos_written);

      memset(outinfo->vs_output_param_offset, AC_EXP_PARAM_UNDEFINED,
             sizeof(outinfo->vs_output_param_offset));

      uint64_t per_prim_mask =
         nir->info.outputs_written & nir->info.per_primitive_outputs &
         ~BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_INDICES) & ~BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_COUNT);
      uint64_t per_vtx_mask =
         nir->info.outputs_written & ~per_prim_mask;

      unsigned total_param_exports = 0;

      /* Per-vertex outputs */
      assign_outinfo_params(outinfo, per_vtx_mask, &total_param_exports);
      if (outinfo->writes_layer)
         assign_outinfo_param(outinfo, VARYING_SLOT_LAYER, &total_param_exports);
      if (outinfo->export_prim_id)
         assign_outinfo_param(outinfo, VARYING_SLOT_PRIMITIVE_ID, &total_param_exports);

      outinfo->param_exports = total_param_exports;

      /* Per-primitive outputs: the HW needs these to be last. */
      assign_outinfo_params(outinfo, per_prim_mask, &total_param_exports);
      if (outinfo->writes_layer_per_primitive)
         assign_outinfo_param(outinfo, VARYING_SLOT_LAYER, &total_param_exports);
      if (outinfo->writes_viewport_index_per_primitive)
         assign_outinfo_param(outinfo, VARYING_SLOT_VIEWPORT, &total_param_exports);
      if (outinfo->export_prim_id_per_primitive)
         assign_outinfo_param(outinfo, VARYING_SLOT_PRIMITIVE_ID, &total_param_exports);

      outinfo->prim_param_exports = total_param_exports - outinfo->param_exports;
   }

   if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      uint64_t per_primitive_input_mask = nir->info.inputs_read & nir->info.per_primitive_inputs;
      unsigned num_per_primitive_inputs = util_bitcount64(per_primitive_input_mask);
      assert(num_per_primitive_inputs <= nir->num_inputs);

      info->ps.num_interp = nir->num_inputs - num_per_primitive_inputs;
      info->ps.num_prim_interp = num_per_primitive_inputs;
   }

   switch (nir->info.stage) {
   case MESA_SHADER_COMPUTE:
      for (int i = 0; i < 3; ++i)
         info->cs.block_size[i] = nir->info.workgroup_size[i];
      break;
   case MESA_SHADER_FRAGMENT:
      info->ps.can_discard = nir->info.fs.uses_discard;
      info->ps.early_fragment_test = nir->info.fs.early_fragment_tests;
      info->ps.post_depth_coverage = nir->info.fs.post_depth_coverage;
      info->ps.depth_layout = nir->info.fs.depth_layout;
      info->ps.uses_sample_shading = nir->info.fs.uses_sample_shading;
      break;
   case MESA_SHADER_GEOMETRY:
      info->gs.vertices_in = nir->info.gs.vertices_in;
      info->gs.vertices_out = nir->info.gs.vertices_out;
      info->gs.output_prim = nir->info.gs.output_primitive;
      info->gs.invocations = nir->info.gs.invocations;
      break;
   case MESA_SHADER_TESS_EVAL:
      info->tes._primitive_mode = nir->info.tess._primitive_mode;
      info->tes.spacing = nir->info.tess.spacing;
      info->tes.ccw = nir->info.tess.ccw;
      info->tes.point_mode = nir->info.tess.point_mode;
      break;
   case MESA_SHADER_TESS_CTRL:
      info->tcs.tcs_vertices_out = nir->info.tess.tcs_vertices_out;
      break;
   case MESA_SHADER_VERTEX:
      break;
   case MESA_SHADER_MESH:
      info->ms.output_prim = nir->info.mesh.primitive_type;
      break;
   default:
      break;
   }

   if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      unsigned add_clip =
         nir->info.clip_distance_array_size + nir->info.cull_distance_array_size > 4;
      info->gs.gsvs_vertex_size = (util_bitcount64(nir->info.outputs_written) + add_clip) * 16;
      info->gs.max_gsvs_emit_size = info->gs.gsvs_vertex_size * nir->info.gs.vertices_out;
   }

   /* Compute the ESGS item size for VS or TES as ES. */
   if ((nir->info.stage == MESA_SHADER_VERTEX && info->vs.as_es) ||
       (nir->info.stage == MESA_SHADER_TESS_EVAL && info->tes.as_es)) {
      struct radv_es_output_info *es_info =
         nir->info.stage == MESA_SHADER_VERTEX ? &info->vs.es_info : &info->tes.es_info;
      uint32_t num_outputs_written = nir->info.stage == MESA_SHADER_VERTEX
                                        ? info->vs.num_linked_outputs
                                        : info->tes.num_linked_outputs;
      es_info->esgs_itemsize = num_outputs_written * 16;
   }

   if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      bool uses_persp_or_linear_interp = info->ps.reads_persp_center ||
                                         info->ps.reads_persp_centroid ||
                                         info->ps.reads_persp_sample ||
                                         info->ps.reads_linear_center ||
                                         info->ps.reads_linear_centroid ||
                                         info->ps.reads_linear_sample;

      info->ps.allow_flat_shading =
         !(uses_persp_or_linear_interp || info->ps.needs_sample_positions ||
           info->ps.writes_memory || nir->info.fs.needs_quad_helper_invocations ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FRAG_COORD) ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_POINT_COORD) ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID) ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_POS) ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_MASK_IN) ||
           BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_HELPER_INVOCATION));

      info->ps.spi_ps_input = radv_compute_spi_ps_input(device, info);
   }
}
