#ifndef __EDITOR_H__
#define __EDITOR_H__
#include "tools.h"
typedef struct dEditorStyle{
    vec4 bg_col;
	vec4 light_bg_col;
	vec4 very_light_bg_col;

    vec4 title_col;

	vec4 panel_col;
	vec4 panel_hover_col;
	vec4 panel_active_col;

	vec4 text_col;
	vec4 text_disabled_col;
	vec4 border_col; 
}dEditorStyle;


typedef struct
dEditor{
    vec4 viewport; //3D viewport bounds
    dEditorStyle style;
}dEditor;


void deditor_init(dEditor *editor);
void deditor_draw(dEditor *editor);
void deditor_update(dEditor *editor, float dt);
#endif //__EDITOR_H__