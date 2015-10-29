#include <pebble.h>
#include "progress_bar.h"

void draw_round_bar(GContext *ctx, GPoint l_p, GPoint r_p, int height) {
	
	graphics_fill_circle(ctx, l_p, height/2);
	graphics_draw_circle(ctx, l_p, height/2);
	
	graphics_fill_circle(ctx, r_p, height/2);
	graphics_draw_circle(ctx, r_p, height/2);
	
	GRect bar = {
		.origin = {l_p.x, 0},
		.size = {r_p.x - l_p.x, height}
	};
	graphics_fill_rect(ctx, bar, 0, GCornerNone);
	
}

void draw_progress_bar(Layer *l, GContext *ctx, float progress, GColor back, GColor greyed, GColor comp) {
	
	if( progress >= 0.0 && progress <= 100.0) {
		
		GRect bounds = layer_get_bounds(l);
		
		int h = bounds.size.h;
		int w = bounds.size.w;
		
		// background fill of the layer
		graphics_context_set_fill_color(ctx, back);
		graphics_fill_rect(ctx, bounds, 0, GCornerNone);
		
		// set colour to draw the greyed-out bar
		graphics_context_set_stroke_color(ctx, greyed);
		graphics_context_set_fill_color(ctx, greyed);
		
		// draw the greyed out bar
		GPoint left_centre = { h/2 , h/2};
		GPoint right_centre = {w - h/2 - 1, h/2};
		draw_round_bar(ctx, left_centre, right_centre, h);
		
		// set colour to draw the filled bar
		graphics_context_set_stroke_color(ctx, comp);
		graphics_context_set_fill_color(ctx, comp);
		
		// draw the completed bar
		int xPos = h/2 + (int)(progress * (w - h - 1) / 100.0);
		if(xPos > right_centre.x) xPos = right_centre.x;
		GPoint middle_point = { xPos, h/2};
		draw_round_bar(ctx, left_centre, middle_point, h);
		
	}
	
}