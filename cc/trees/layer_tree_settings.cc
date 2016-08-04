// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_settings.h"

#include "cc/proto/gfx_conversions.h"
#include "cc/proto/layer_tree_settings.pb.h"
#include "third_party/khronos/GLES2/gl2.h"

namespace cc {

namespace {

proto::LayerTreeSettings_ScrollbarAnimator
LayerTreeSettingsScrollbarAnimatorToProto(
    const LayerTreeSettings::ScrollbarAnimator& animator) {
  switch (animator) {
    case LayerTreeSettings::ScrollbarAnimator::NO_ANIMATOR:
      return proto::LayerTreeSettings_ScrollbarAnimator_NO_ANIMATOR;
    case LayerTreeSettings::ScrollbarAnimator::LINEAR_FADE:
      return proto::LayerTreeSettings_ScrollbarAnimator_LINEAR_FADE;
    case LayerTreeSettings::ScrollbarAnimator::THINNING:
      return proto::LayerTreeSettings_ScrollbarAnimator_THINNING;
  }
  NOTREACHED() << "proto::LayerTreeSettings_ScrollbarAnimator_UNKNOWN";
  return proto::LayerTreeSettings_ScrollbarAnimator_UNKNOWN;
}

LayerTreeSettings::ScrollbarAnimator
LayerTreeSettingsScrollbarAnimatorFromProto(
    const proto::LayerTreeSettings_ScrollbarAnimator& animator) {
  switch (animator) {
    case proto::LayerTreeSettings_ScrollbarAnimator_NO_ANIMATOR:
      return LayerTreeSettings::ScrollbarAnimator::NO_ANIMATOR;
    case proto::LayerTreeSettings_ScrollbarAnimator_LINEAR_FADE:
      return LayerTreeSettings::ScrollbarAnimator::LINEAR_FADE;
    case proto::LayerTreeSettings_ScrollbarAnimator_THINNING:
      return LayerTreeSettings::ScrollbarAnimator::THINNING;
    case proto::LayerTreeSettings_ScrollbarAnimator_UNKNOWN:
      NOTREACHED() << "proto::LayerTreeSettings_ScrollbarAnimator_UNKNOWN";
      return LayerTreeSettings::ScrollbarAnimator::NO_ANIMATOR;
  }
  return LayerTreeSettings::ScrollbarAnimator::NO_ANIMATOR;
}

}  // namespace

LayerTreeSettings::LayerTreeSettings()
    : default_tile_size(gfx::Size(256, 256)),
      max_untiled_layer_size(gfx::Size(512, 512)),
      minimum_occlusion_tracking_size(gfx::Size(160, 160)),
      memory_policy_(64 * 1024 * 1024,
                     gpu::MemoryAllocation::CUTOFF_ALLOW_EVERYTHING,
                     ManagedMemoryPolicy::kDefaultNumResourcesLimit) {}

LayerTreeSettings::LayerTreeSettings(const LayerTreeSettings& other) = default;
LayerTreeSettings::~LayerTreeSettings() = default;

bool LayerTreeSettings::operator==(const LayerTreeSettings& other) const {
  return renderer_settings == other.renderer_settings &&
         single_thread_proxy_scheduler == other.single_thread_proxy_scheduler &&
         use_external_begin_frame_source ==
             other.use_external_begin_frame_source &&
         main_frame_before_activation_enabled ==
             other.main_frame_before_activation_enabled &&
         using_synchronous_renderer_compositor ==
             other.using_synchronous_renderer_compositor &&
         can_use_lcd_text == other.can_use_lcd_text &&
         use_distance_field_text == other.use_distance_field_text &&
         gpu_rasterization_enabled == other.gpu_rasterization_enabled &&
         gpu_rasterization_forced == other.gpu_rasterization_forced &&
         async_worker_context_enabled == other.async_worker_context_enabled &&
         gpu_rasterization_msaa_sample_count ==
             other.gpu_rasterization_msaa_sample_count &&
         create_low_res_tiling == other.create_low_res_tiling &&
         scrollbar_animator == other.scrollbar_animator &&
         scrollbar_fade_delay_ms == other.scrollbar_fade_delay_ms &&
         scrollbar_fade_resize_delay_ms ==
             other.scrollbar_fade_resize_delay_ms &&
         scrollbar_fade_duration_ms == other.scrollbar_fade_duration_ms &&
         solid_color_scrollbar_color == other.solid_color_scrollbar_color &&
         timeout_and_draw_when_animation_checkerboards ==
             other.timeout_and_draw_when_animation_checkerboards &&
         layer_transforms_should_scale_layer_contents ==
             other.layer_transforms_should_scale_layer_contents &&
         layers_always_allowed_lcd_text ==
             other.layers_always_allowed_lcd_text &&
         minimum_contents_scale == other.minimum_contents_scale &&
         low_res_contents_scale_factor == other.low_res_contents_scale_factor &&
         top_controls_show_threshold == other.top_controls_show_threshold &&
         top_controls_hide_threshold == other.top_controls_hide_threshold &&
         background_animation_rate == other.background_animation_rate &&
         default_tile_size == other.default_tile_size &&
         max_untiled_layer_size == other.max_untiled_layer_size &&
         minimum_occlusion_tracking_size ==
             other.minimum_occlusion_tracking_size &&
         tiling_interest_area_padding == other.tiling_interest_area_padding &&
         skewport_target_time_in_seconds ==
             other.skewport_target_time_in_seconds &&
         skewport_extrapolation_limit_in_screen_pixels ==
             other.skewport_extrapolation_limit_in_screen_pixels &&
         max_memory_for_prepaint_percentage ==
             other.max_memory_for_prepaint_percentage &&
         use_zero_copy == other.use_zero_copy &&
         use_partial_raster == other.use_partial_raster &&
         enable_elastic_overscroll == other.enable_elastic_overscroll &&
         ignore_root_layer_flings == other.ignore_root_layer_flings &&
         scheduled_raster_task_limit == other.scheduled_raster_task_limit &&
         use_occlusion_for_tile_prioritization ==
             other.use_occlusion_for_tile_prioritization &&
         verify_clip_tree_calculations == other.verify_clip_tree_calculations &&
         verify_transform_tree_calculations ==
             other.verify_transform_tree_calculations &&
         image_decode_tasks_enabled == other.image_decode_tasks_enabled &&
         wait_for_beginframe_interval == other.wait_for_beginframe_interval &&
         max_staging_buffer_usage_in_bytes ==
             other.max_staging_buffer_usage_in_bytes &&
         memory_policy_ == other.memory_policy_ &&
         LayerTreeDebugState::Equal(initial_debug_state,
                                    other.initial_debug_state) &&
         use_cached_picture_raster == other.use_cached_picture_raster;
}

void LayerTreeSettings::ToProtobuf(proto::LayerTreeSettings* proto) const {
  renderer_settings.ToProtobuf(proto->mutable_renderer_settings());
  proto->set_single_thread_proxy_scheduler(single_thread_proxy_scheduler);
  proto->set_use_external_begin_frame_source(use_external_begin_frame_source);
  proto->set_main_frame_before_activation_enabled(
      main_frame_before_activation_enabled);
  proto->set_using_synchronous_renderer_compositor(
      using_synchronous_renderer_compositor);
  proto->set_can_use_lcd_text(can_use_lcd_text);
  proto->set_use_distance_field_text(use_distance_field_text);
  proto->set_gpu_rasterization_enabled(gpu_rasterization_enabled);
  proto->set_gpu_rasterization_forced(gpu_rasterization_forced);
  proto->set_async_worker_context_enabled(async_worker_context_enabled);
  proto->set_gpu_rasterization_msaa_sample_count(
      gpu_rasterization_msaa_sample_count);
  proto->set_create_low_res_tiling(create_low_res_tiling);
  proto->set_scrollbar_animator(
      LayerTreeSettingsScrollbarAnimatorToProto(scrollbar_animator));
  proto->set_scrollbar_fade_delay_ms(scrollbar_fade_delay_ms);
  proto->set_scrollbar_fade_resize_delay_ms(scrollbar_fade_resize_delay_ms);
  proto->set_scrollbar_fade_duration_ms(scrollbar_fade_duration_ms);
  proto->set_solid_color_scrollbar_color(solid_color_scrollbar_color);
  proto->set_timeout_and_draw_when_animation_checkerboards(
      timeout_and_draw_when_animation_checkerboards);
  proto->set_layer_transforms_should_scale_layer_contents(
      layer_transforms_should_scale_layer_contents);
  proto->set_layers_always_allowed_lcd_text(layers_always_allowed_lcd_text);
  proto->set_minimum_contents_scale(minimum_contents_scale);
  proto->set_low_res_contents_scale_factor(low_res_contents_scale_factor);
  proto->set_top_controls_show_threshold(top_controls_show_threshold);
  proto->set_top_controls_hide_threshold(top_controls_hide_threshold);
  proto->set_background_animation_rate(background_animation_rate);
  SizeToProto(default_tile_size, proto->mutable_default_tile_size());
  SizeToProto(max_untiled_layer_size, proto->mutable_max_untiled_layer_size());
  SizeToProto(minimum_occlusion_tracking_size,
              proto->mutable_minimum_occlusion_tracking_size());
  proto->set_tiling_interest_area_padding(tiling_interest_area_padding);
  proto->set_skewport_target_time_in_seconds(skewport_target_time_in_seconds);
  proto->set_skewport_extrapolation_limit_in_screen_pixels(
      skewport_extrapolation_limit_in_screen_pixels);
  proto->set_max_memory_for_prepaint_percentage(
      max_memory_for_prepaint_percentage);
  proto->set_use_zero_copy(use_zero_copy);
  proto->set_use_partial_raster(use_partial_raster);
  proto->set_enable_elastic_overscroll(enable_elastic_overscroll);
  proto->set_ignore_root_layer_flings(ignore_root_layer_flings);
  proto->set_scheduled_raster_task_limit(scheduled_raster_task_limit);
  proto->set_use_occlusion_for_tile_prioritization(
      use_occlusion_for_tile_prioritization);
  proto->set_image_decode_tasks_enabled(image_decode_tasks_enabled);
  proto->set_wait_for_beginframe_interval(wait_for_beginframe_interval);
  proto->set_max_staging_buffer_usage_in_bytes(
      max_staging_buffer_usage_in_bytes);
  memory_policy_.ToProtobuf(proto->mutable_memory_policy());
  initial_debug_state.ToProtobuf(proto->mutable_initial_debug_state());
  proto->set_use_cached_picture_raster(use_cached_picture_raster);
}

void LayerTreeSettings::FromProtobuf(const proto::LayerTreeSettings& proto) {
  renderer_settings.FromProtobuf(proto.renderer_settings());
  single_thread_proxy_scheduler = proto.single_thread_proxy_scheduler();
  use_external_begin_frame_source = proto.use_external_begin_frame_source();
  main_frame_before_activation_enabled =
      proto.main_frame_before_activation_enabled();
  using_synchronous_renderer_compositor =
      proto.using_synchronous_renderer_compositor();
  can_use_lcd_text = proto.can_use_lcd_text();
  use_distance_field_text = proto.use_distance_field_text();
  gpu_rasterization_enabled = proto.gpu_rasterization_enabled();
  gpu_rasterization_forced = proto.gpu_rasterization_forced();
  async_worker_context_enabled = proto.async_worker_context_enabled();
  gpu_rasterization_msaa_sample_count =
      proto.gpu_rasterization_msaa_sample_count();
  create_low_res_tiling = proto.create_low_res_tiling();
  scrollbar_animator =
      LayerTreeSettingsScrollbarAnimatorFromProto(proto.scrollbar_animator());
  scrollbar_fade_delay_ms = proto.scrollbar_fade_delay_ms();
  scrollbar_fade_resize_delay_ms = proto.scrollbar_fade_resize_delay_ms();
  scrollbar_fade_duration_ms = proto.scrollbar_fade_duration_ms();
  solid_color_scrollbar_color = proto.solid_color_scrollbar_color();
  timeout_and_draw_when_animation_checkerboards =
      proto.timeout_and_draw_when_animation_checkerboards();
  layer_transforms_should_scale_layer_contents =
      proto.layer_transforms_should_scale_layer_contents();
  layers_always_allowed_lcd_text = proto.layers_always_allowed_lcd_text();
  minimum_contents_scale = proto.minimum_contents_scale();
  low_res_contents_scale_factor = proto.low_res_contents_scale_factor();
  top_controls_show_threshold = proto.top_controls_show_threshold();
  top_controls_hide_threshold = proto.top_controls_hide_threshold();
  background_animation_rate = proto.background_animation_rate();
  default_tile_size = ProtoToSize(proto.default_tile_size());
  max_untiled_layer_size = ProtoToSize(proto.max_untiled_layer_size());
  minimum_occlusion_tracking_size =
      ProtoToSize(proto.minimum_occlusion_tracking_size());
  tiling_interest_area_padding = proto.tiling_interest_area_padding();
  skewport_target_time_in_seconds = proto.skewport_target_time_in_seconds();
  skewport_extrapolation_limit_in_screen_pixels =
      proto.skewport_extrapolation_limit_in_screen_pixels();
  max_memory_for_prepaint_percentage =
      proto.max_memory_for_prepaint_percentage();
  use_zero_copy = proto.use_zero_copy();
  use_partial_raster = proto.use_partial_raster();
  enable_elastic_overscroll = proto.enable_elastic_overscroll();
  ignore_root_layer_flings = proto.ignore_root_layer_flings();
  scheduled_raster_task_limit = proto.scheduled_raster_task_limit();
  use_occlusion_for_tile_prioritization =
      proto.use_occlusion_for_tile_prioritization();
  image_decode_tasks_enabled = proto.image_decode_tasks_enabled();
  wait_for_beginframe_interval = proto.wait_for_beginframe_interval();
  max_staging_buffer_usage_in_bytes = proto.max_staging_buffer_usage_in_bytes();
  memory_policy_.FromProtobuf(proto.memory_policy());
  initial_debug_state.FromProtobuf(proto.initial_debug_state());
  use_cached_picture_raster = proto.use_cached_picture_raster();
}

SchedulerSettings LayerTreeSettings::ToSchedulerSettings() const {
  SchedulerSettings scheduler_settings;
  scheduler_settings.use_external_begin_frame_source =
      use_external_begin_frame_source;
  scheduler_settings.main_frame_before_activation_enabled =
      main_frame_before_activation_enabled;
  scheduler_settings.timeout_and_draw_when_animation_checkerboards =
      timeout_and_draw_when_animation_checkerboards;
  scheduler_settings.using_synchronous_renderer_compositor =
      using_synchronous_renderer_compositor;
  scheduler_settings.throttle_frame_production = wait_for_beginframe_interval;
  scheduler_settings.background_frame_interval =
      base::TimeDelta::FromSecondsD(1.0 / background_animation_rate);
  scheduler_settings.abort_commit_before_output_surface_creation =
      abort_commit_before_output_surface_creation;
  return scheduler_settings;
}

LayerTreeSettings::LayerTreeSettings(cc::mojom::LayerTreeSettings* m)
    : memory_policy_(m->memory_policy_->bytes_limit_when_visible,
                     static_cast<gpu::MemoryAllocation::PriorityCutoff>(
                         m->memory_policy_->priority_cutoff_when_visible),
                     m->memory_policy_->num_resources_limit) {
  main_frame_before_activation_enabled =
      m->main_frame_before_activation_enabled;
  can_use_lcd_text = m->can_use_lcd_text;
  use_distance_field_text = m->use_distance_field_text;
  gpu_rasterization_enabled = m->gpu_rasterization_enabled;
  gpu_rasterization_forced = m->gpu_rasterization_forced;
  async_worker_context_enabled = m->async_worker_context_enabled;
  gpu_rasterization_msaa_sample_count = m->gpu_rasterization_msaa_sample_count;
  gpu_rasterization_skewport_target_time_in_seconds =
      m->gpu_rasterization_skewport_target_time_in_seconds;
  create_low_res_tiling = m->create_low_res_tiling;
  scrollbar_animator = static_cast<ScrollbarAnimator>(m->scrollbar_animator);
  scrollbar_fade_delay_ms = m->scrollbar_fade_delay_ms;
  scrollbar_fade_resize_delay_ms = m->scrollbar_fade_resize_delay_ms;
  scrollbar_fade_duration_ms = m->scrollbar_fade_duration_ms;
  solid_color_scrollbar_color = m->solid_color_scrollbar_color;
  timeout_and_draw_when_animation_checkerboards =
      m->timeout_and_draw_when_animation_checkerboards;
  layer_transforms_should_scale_layer_contents =
      m->layer_transforms_should_scale_layer_contents;
  layers_always_allowed_lcd_text = m->layers_always_allowed_lcd_text;
  minimum_contents_scale = m->minimum_contents_scale;
  low_res_contents_scale_factor = m->low_res_contents_scale_factor;
  top_controls_show_threshold = m->top_controls_show_threshold;
  top_controls_hide_threshold = m->top_controls_hide_threshold;
  background_animation_rate = m->background_animation_rate;
  tiling_interest_area_padding = m->tiling_interest_area_padding;
  skewport_target_time_in_seconds = m->skewport_target_time_in_seconds;
  skewport_extrapolation_limit_in_screen_pixels =
      m->skewport_extrapolation_limit_in_screen_pixels;
  max_memory_for_prepaint_percentage = m->max_memory_for_prepaint_percentage;
  use_zero_copy = m->use_zero_copy;
  use_partial_raster = m->use_partial_raster;
  enable_elastic_overscroll = m->enable_elastic_overscroll;
  ignore_root_layer_flings = m->ignore_root_layer_flings;
  scheduled_raster_task_limit = m->scheduled_raster_task_limit;
  use_occlusion_for_tile_prioritization =
      m->use_occlusion_for_tile_prioritization;
  verify_clip_tree_calculations = m->verify_clip_tree_calculations;
  verify_transform_tree_calculations = m->verify_transform_tree_calculations;
  image_decode_tasks_enabled = m->image_decode_tasks_enabled;
  wait_for_beginframe_interval = m->wait_for_beginframe_interval;
  abort_commit_before_output_surface_creation =
      m->abort_commit_before_output_surface_creation;
  use_layer_lists = m->use_layer_lists;
  max_staging_buffer_usage_in_bytes = m->max_staging_buffer_usage_in_bytes;
  gpu_decoded_image_budget_bytes = m->gpu_decoded_image_budget_bytes;
  software_decoded_image_budget_bytes = m->software_decoded_image_budget_bytes;
  max_preraster_distance_in_screen_pixels =
      m->max_preraster_distance_in_screen_pixels;
  use_cached_picture_raster = m->use_cached_picture_raster;
  default_tile_size = m->default_tile_size;
  max_untiled_layer_size = m->max_untiled_layer_size;
  minimum_occlusion_tracking_size = m->minimum_occlusion_tracking_size;

  initial_debug_state.show_fps_counter = m->show_fps_counter;
  initial_debug_state.show_debug_borders = m->show_debug_borders;
  initial_debug_state.show_paint_rects = m->show_paint_rects;
  initial_debug_state.show_property_changed_rects = m->show_property_changed_rects;
  initial_debug_state.show_surface_damage_rects = m->show_surface_damage_rects;
  initial_debug_state.show_screen_space_rects = m->show_screen_space_rects;
  initial_debug_state.show_replica_screen_space_rects = m->show_replica_screen_space_rects;
  initial_debug_state.show_touch_event_handler_rects = m->show_touch_event_handler_rects;
  initial_debug_state.show_wheel_event_handler_rects = m->show_wheel_event_handler_rects;
  initial_debug_state.show_scroll_event_handler_rects = m->show_scroll_event_handler_rects;
  initial_debug_state.show_non_fast_scrollable_rects = m->show_non_fast_scrollable_rects;
  initial_debug_state.show_layer_animation_bounds_rects = m->show_layer_animation_bounds_rects;
  initial_debug_state.slow_down_raster_scale_factor = m->slow_down_raster_scale_factor;
  initial_debug_state.rasterize_only_visible_content = m->rasterize_only_visible_content;
  initial_debug_state.show_picture_borders = m->show_picture_borders;

  renderer_settings.allow_antialiasing = m->allow_antialiasing;
  renderer_settings.force_antialiasing = m->force_antialiasing;
  renderer_settings.force_blending_with_shaders = m->force_blending_with_shaders;
  renderer_settings.partial_swap_enabled = m->partial_swap_enabled;
  renderer_settings.finish_rendering_on_resize = m->finish_rendering_on_resize;
  renderer_settings.should_clear_root_render_pass = m->should_clear_root_render_pass;
  renderer_settings.disable_display_vsync = m->disable_display_vsync;
  renderer_settings.release_overlay_resources_after_gpu_query = m->release_overlay_resources_after_gpu_query;
  renderer_settings.refresh_rate = m->refresh_rate;
  renderer_settings.highp_threshold_min = m->highp_threshold_min;
  renderer_settings.texture_id_allocation_chunk_size = m->texture_id_allocation_chunk_size;
  renderer_settings.use_gpu_memory_buffer_resources = m->use_gpu_memory_buffer_resources;
  renderer_settings.preferred_tile_format = static_cast<ResourceFormat>(m->preferred_tile_format);

  DCHECK_EQ(m->buffer_types.size(), m->texture_targets.size());
  const size_t n = m->buffer_types.size();
  for (size_t i = 0; i < n; ++i) {
    renderer_settings.buffer_to_texture_target_map[std::make_pair(
        static_cast<gfx::BufferUsage>(m->buffer_types[i]->usage),
        static_cast<gfx::BufferFormat>(m->buffer_types[i]->format))] =
        m->texture_targets[i];
  }
}

cc::mojom::LayerTreeSettingsPtr LayerTreeSettings::ToMojom() const {
  auto m = cc::mojom::LayerTreeSettings::New();

  m->memory_policy_ = cc::mojom::ManagedMemoryPolicy::New();
  m->memory_policy_->bytes_limit_when_visible = memory_policy_.bytes_limit_when_visible;
  m->memory_policy_->priority_cutoff_when_visible =
      static_cast<gpu::mojom::MemoryAllocationPriorityCutoff>(
          memory_policy_.priority_cutoff_when_visible);
  m->memory_policy_->num_resources_limit = memory_policy_.num_resources_limit;
  m->main_frame_before_activation_enabled =
      main_frame_before_activation_enabled;
  m->can_use_lcd_text = can_use_lcd_text;
  m->use_distance_field_text = use_distance_field_text;
  m->gpu_rasterization_enabled = gpu_rasterization_enabled;
  m->gpu_rasterization_forced = gpu_rasterization_forced;
  m->async_worker_context_enabled = async_worker_context_enabled;
  m->gpu_rasterization_msaa_sample_count = gpu_rasterization_msaa_sample_count;
  m->gpu_rasterization_skewport_target_time_in_seconds =
      gpu_rasterization_skewport_target_time_in_seconds;
  m->create_low_res_tiling = create_low_res_tiling;
  m->scrollbar_animator = static_cast<cc::mojom::ScrollbarAnimator>(scrollbar_animator);
  m->scrollbar_fade_delay_ms = scrollbar_fade_delay_ms;
  m->scrollbar_fade_resize_delay_ms = scrollbar_fade_resize_delay_ms;
  m->scrollbar_fade_duration_ms = scrollbar_fade_duration_ms;
  m->solid_color_scrollbar_color = solid_color_scrollbar_color;
  m->timeout_and_draw_when_animation_checkerboards =
      timeout_and_draw_when_animation_checkerboards;
  m->layer_transforms_should_scale_layer_contents =
      layer_transforms_should_scale_layer_contents;
  m->layers_always_allowed_lcd_text = layers_always_allowed_lcd_text;
  m->minimum_contents_scale = minimum_contents_scale;
  m->low_res_contents_scale_factor = low_res_contents_scale_factor;
  m->top_controls_show_threshold = top_controls_show_threshold;
  m->top_controls_hide_threshold = top_controls_hide_threshold;
  m->background_animation_rate = background_animation_rate;
  m->tiling_interest_area_padding = tiling_interest_area_padding;
  m->skewport_target_time_in_seconds = skewport_target_time_in_seconds;
  m->skewport_extrapolation_limit_in_screen_pixels =
      skewport_extrapolation_limit_in_screen_pixels;
  m->max_memory_for_prepaint_percentage = max_memory_for_prepaint_percentage;
  m->use_zero_copy = use_zero_copy;
  m->use_partial_raster = use_partial_raster;
  m->enable_elastic_overscroll = enable_elastic_overscroll;
  m->ignore_root_layer_flings = ignore_root_layer_flings;
  m->scheduled_raster_task_limit = scheduled_raster_task_limit;
  m->use_occlusion_for_tile_prioritization =
      use_occlusion_for_tile_prioritization;
  m->verify_clip_tree_calculations = verify_clip_tree_calculations;
  m->verify_transform_tree_calculations = verify_transform_tree_calculations;
  m->image_decode_tasks_enabled = image_decode_tasks_enabled;
  m->wait_for_beginframe_interval = wait_for_beginframe_interval;
  m->abort_commit_before_output_surface_creation =
      abort_commit_before_output_surface_creation;
  m->use_layer_lists = use_layer_lists;
  m->max_staging_buffer_usage_in_bytes = max_staging_buffer_usage_in_bytes;
  m->gpu_decoded_image_budget_bytes = gpu_decoded_image_budget_bytes;
  m->software_decoded_image_budget_bytes = software_decoded_image_budget_bytes;
  m->max_preraster_distance_in_screen_pixels =
      max_preraster_distance_in_screen_pixels;
  m->use_cached_picture_raster = use_cached_picture_raster;
  m->default_tile_size = default_tile_size;
  m->max_untiled_layer_size = max_untiled_layer_size;
  m->minimum_occlusion_tracking_size = minimum_occlusion_tracking_size;

  m->show_fps_counter = initial_debug_state.show_fps_counter;
  m->show_debug_borders = initial_debug_state.show_debug_borders;
  m->show_paint_rects = initial_debug_state.show_paint_rects;
  m->show_property_changed_rects = initial_debug_state.show_property_changed_rects;
  m->show_surface_damage_rects = initial_debug_state.show_surface_damage_rects;
  m->show_screen_space_rects = initial_debug_state.show_screen_space_rects;
  m->show_replica_screen_space_rects = initial_debug_state.show_replica_screen_space_rects;
  m->show_touch_event_handler_rects = initial_debug_state.show_touch_event_handler_rects;
  m->show_wheel_event_handler_rects = initial_debug_state.show_wheel_event_handler_rects;
  m->show_scroll_event_handler_rects = initial_debug_state.show_scroll_event_handler_rects;
  m->show_non_fast_scrollable_rects = initial_debug_state.show_non_fast_scrollable_rects;
  m->show_layer_animation_bounds_rects = initial_debug_state.show_layer_animation_bounds_rects;
  m->slow_down_raster_scale_factor = initial_debug_state.slow_down_raster_scale_factor;
  m->rasterize_only_visible_content = initial_debug_state.rasterize_only_visible_content;
  m->show_picture_borders = initial_debug_state.show_picture_borders;

  m->allow_antialiasing = renderer_settings.allow_antialiasing;
  m->force_antialiasing = renderer_settings.force_antialiasing;
  m->force_blending_with_shaders = renderer_settings.force_blending_with_shaders;
  m->partial_swap_enabled = renderer_settings.partial_swap_enabled;
  m->finish_rendering_on_resize = renderer_settings.finish_rendering_on_resize;
  m->should_clear_root_render_pass = renderer_settings.should_clear_root_render_pass;
  m->disable_display_vsync = renderer_settings.disable_display_vsync;
  m->release_overlay_resources_after_gpu_query = renderer_settings.release_overlay_resources_after_gpu_query;
  m->refresh_rate = renderer_settings.refresh_rate;
  m->highp_threshold_min = renderer_settings.highp_threshold_min;
  m->texture_id_allocation_chunk_size = renderer_settings.texture_id_allocation_chunk_size;
  m->use_gpu_memory_buffer_resources = renderer_settings.use_gpu_memory_buffer_resources;
  m->preferred_tile_format = renderer_settings.preferred_tile_format;

  for (const auto& pair : renderer_settings.buffer_to_texture_target_map) {
    auto buffer_types = cc::mojom::BufferUsageFormat::New();
    buffer_types->usage = static_cast<gfx::mojom::BufferUsage>(pair.first.first);
    buffer_types->format = static_cast<gfx::mojom::BufferFormat>(pair.first.second);
    m->buffer_types.push_back(std::move(buffer_types));
    m->texture_targets.push_back(pair.second);
  }

  return m;
}

}  // namespace cc
