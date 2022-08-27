#ifndef __EDITOR_H__
#define __EDITOR_H__

typedef struct
dEditor{
    int viewport[4];
    int viewport3d[4];
    void *v_data;    
}dEditor;

void deditor_init(dEditor *editor);
void deditor_draw(dEditor *editor);
void deditor_update(dEditor *editor, float dt);
#endif //__EDITOR_H__