#include <pebble.h>

#define COLOR(x) {.argb=GColor ## x ## ARGB8}

typedef struct Segment {
  int inner;
  int outer;
  int angle;
  int width;
  GColor color;
} Segment;

Segment segments[17] = {0};

const int segment_count = (int)(sizeof(segments) / sizeof(Segment));
static AppTimer *radial_timer = NULL;
static Layer *canvas_layer = NULL;

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  for (int i = 0; i < segment_count; i++) {
    Segment *segment = &segments[i];

    int direction = (i % 2) ? -1 : 1;
    int speed = (segment_count - i + 1);
    segment->angle = (segment->angle + direction * speed) % 360;
    int angle_start = (segment->angle * TRIG_MAX_ANGLE) / 360;
    int angle_end = angle_start + (segment->width * TRIG_MAX_ANGLE) / 360;
    graphics_context_set_fill_color(ctx, segment->color);

    graphics_fill_radial(
        ctx, grect_inset(bounds, GEdgeInsets(90 - segment->outer)), 
        GOvalScaleModeFillCircle,
        segment->outer - segment->inner + 1, 
        angle_start, angle_end);
  }
}

static void canvas_update_timer(void* data) {
  radial_timer = app_timer_register(80, canvas_update_timer, data);
  layer_mark_dirty(canvas_layer);
}

void radial_setup(Layer *_canvas_layer) {
  const int distance = 5;
  const int width = 10;
  for (int i = 0; i < segment_count; i++) {
    Segment *segment = &segments[i];
    *segment = (Segment){
      .inner = i * distance, .outer = i * distance + distance, 
      .angle = 0, .width = width + i, .color = COLOR(VividCerulean)
    };
  }
  canvas_layer = _canvas_layer;
  layer_set_update_proc(canvas_layer, canvas_update_proc);
}

void radial_stop(void) {
  if (radial_timer) {
    app_timer_cancel(radial_timer);
  }
}

void radial_start(void) {
  if (radial_timer) {
    app_timer_cancel(radial_timer);
  }
  radial_timer = app_timer_register(80, canvas_update_timer, canvas_layer);
}
