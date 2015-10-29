#pragma once

/*	Public API for the progress bar
 *	
 *	Arguments to be provided:
 *		- the layer into which the bar is to be drawn (it is assumed that the bar fills the bounds of that layer)
 *		- the context of that layer, with fill colour set for the SCREEN background and stroke set to the BAR colour
 *		- the progress, as a float, with value between 0.0 and 100.0
 *		- colours for background, greyed-out and complete bar
 */

void draw_progress_bar(Layer *l, GContext *ctx, float progress, GColor back, GColor greyed, GColor comp);