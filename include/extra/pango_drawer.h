#ifndef LABEL_CATHETERE_PANGO_DRAWER
#define LABEL_CATHETERE_PANGO_DRAWER

#include <pangolin/pangolin.h>
#include <pangolin/gl.h>
#include <pangolin/glsl.h>

#include "../bspline.h"

using namespace Eigen;
using namespace pangolin;

class GLViewCvt
{
public:
    GLViewCvt(uint32_t const w, uint32_t const h, bool const use_typical_coord = false, int level = 0)
        : w(w), h(h), use_typical_coord(use_typical_coord), level(level),
          intensity_scale(1.0),
          Pixel_scale(4*1.0/max(w, h))
    {}

    void WindowToImage(const Viewport& v, int wx, int wy, float& ix, float& iy) const
    {
        ix = w * (wx - v.l) /(float)v.w - 0.5;
        iy = h * (wy - v.b) /(float)v.h - 0.5;
        ix = std::max(0.0f,std::min(ix, w-1.0f));
        iy = std::max(0.0f,std::min(iy, h-1.0f));

        if(use_typical_coord)
            iy = h - iy - 1.0f;
    }

    void ImageToWindow(const Viewport& v, float ix, float iy, float& wx, float& wy) const
    {
        if(use_typical_coord)
            iy = h - iy - 1.0f;

        wx = v.l + (float)v.w * ix / w;
        wy = v.b + (float)v.h * iy / h;
    }

    Vector2f ImageToNDC(Vector2f const img_pt) const
    {
        return Vector2f((1<<level) * (img_pt[0]+0.5) * 2.0 / w - 1,
                        (1<<level) * ((use_typical_coord ? (h - img_pt[1] - 1.0f) : img_pt[1])+0.5) * 2.0 / h - 1);
    }

    float GetPixelScale() const
    {
        return Pixel_scale;
    }

    void SetPixelScale(float scale)
    {
        Pixel_scale = scale;
    }

    int GetLevel() const
    {
        return level;
    }

    uint32_t GetWidth() const { return w; }
    uint32_t GetHeight() const { return h; }

    bool IsTypicalCoord() const { return use_typical_coord; }

protected:

    uint32_t w, h;

    float intensity_scale;
    float Pixel_scale;
    int level;

    bool use_typical_coord;

};

/* Flip Y from the handler! */
class Handler2D : public Handler
{
public:
    Handler2D(GLViewCvt const& view_img_cvt)
        : view_img_cvt(view_img_cvt)
    {
        has_picked_pt = false;
        has_selected_roi = false;

        selected_roi = Vector4f::Zero();
        picked_pt = Vector2f::Zero();
    }

    virtual void Keyboard(View&, unsigned char key, int /*x*/, int /*y*/, bool /*pressed*/)
    {
        if(key == 'r') {
            has_picked_pt = false;
            picked_pt = Vector2f::Zero();

            has_selected_roi = false;
            selected_roi = Vector4f::Zero();
        }
    }

    virtual void Mouse(View& view, MouseButton button, int x, int y, bool pressed, int /*button_state*/)
    {

        if(pressed && button == MouseButtonLeft) {
            view_img_cvt.WindowToImage(view.v, x, y, picked_pt[0], picked_pt[1]);
            has_picked_pt = true;
        }

        if(pressed && button == MouseButtonRight) {
            view_img_cvt.WindowToImage(view.v, x, y, selected_roi[0], selected_roi[1]);
            selected_roi[2] = selected_roi[0];
            selected_roi[3] = selected_roi[1];
            has_selected_roi = true;
        } else if(!pressed && button == MouseButtonRight) {
            view_img_cvt.WindowToImage(view.v, x, y, selected_roi[2], selected_roi[3]);
            has_selected_roi = false;
        }
    }

    virtual void MouseMotion(View& view, int x, int y, int /*button_state*/)
    {
        if(has_selected_roi)
            view_img_cvt.WindowToImage(view.v, x, y, selected_roi[2], selected_roi[3]);
    }


    Vector4f GetSelectedROI()
    {
        /* Selected ROI is assumed top-left origin! */
        float x = min(selected_roi[0], selected_roi[2]);
        float y = min(selected_roi[1], selected_roi[3]);
        float w = max(selected_roi[0], selected_roi[2]) - x;
        float h = max(selected_roi[1], selected_roi[3]) - y;

        return Vector4f(x, y, w, h);
    }

    bool HasSelectedROI() const {
        return has_selected_roi;
    }

    Vector2f GetPickedPt()
    {
        has_picked_pt = false;
        return Vector2f(picked_pt[0], picked_pt[1]);
    }

    bool HasPickedPt() const {
        return has_picked_pt;
    }

private:

    GLViewCvt const& view_img_cvt;

    bool has_selected_roi;
    Vector4f selected_roi;

    bool has_picked_pt;
    Vector2f picked_pt;

};

float colour_knot_pt[3] = {1.0, 0.0, 0.0};
float colour_ctrl_pt[3] = {0.0, 1.0, 1.0};
float colour_spline[3] = {1.0, 1.0, 1.0};
float colour_pipe_contour[3] = {0.0, 1.0, 0.0};
float colour_pipe_contour_pts[3] = {1.0, 1.0, 0.0};

inline void glVertex( const Eigen::Vector2f& p ) { glVertex2fv(p.data()); }
inline void glVertex( const Eigen::Vector2d& p ) { glVertex2dv(p.data()); }

inline void glVertex( const Eigen::Vector3f& p ) { glVertex3fv(p.data()); }
inline void glVertex( const Eigen::Vector3d& p ) { glVertex3dv(p.data()); }

class DrawingRoutine
{
public:
    DrawingRoutine() {}

    void operator()(View& view) {

        for(auto f : draw_funcs) f(view);
    }

    vector<boostd::function<void(View&)> > draw_funcs;

};

class DrawTexture
{
public:
    DrawTexture(GLViewCvt const& gl_view_cvt, GlTexture const& tex)
        : gl_view_cvt(gl_view_cvt), tex(tex){}

    void operator()(pangolin::View& view) {

        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glColor4f(1.0, 1.0, 1.0, 1.0);

        view.Activate();

        tex.RenderToViewport(gl_view_cvt.IsTypicalCoord());

        glPopAttrib();
    }

private:
    GLViewCvt const& gl_view_cvt;
    GlTexture const& tex;
};

template<typename _Tp, int dim>
class DrawBSpline
{
public:
    DrawBSpline(GLViewCvt const& view_img_cvt, Bspline<_Tp,dim> const& bspline)
        : gl_view_cvt(view_img_cvt), bspline(bspline), offset(Vector2f::Zero()),
          show_ctrl_pts(true), show_knot_pts(true), show_bspline(true)
    {}

    void ShowCtrlPts(bool const show) { show_ctrl_pts = show; }
    void ShowKnotPts(bool const show) { show_knot_pts = show; }
    void ShowBspline(bool const show) { show_bspline = show; }

    void SetOffset(Vector2f const& offset) {
        this->offset = offset;
    }

    void DrawCtrlPts()
    {
        for(size_t k = 0; k < bspline.GetNumCtrlPts(); ++k)
        {

            Vector2f pt = bspline.GetCtrlPt(k);
            pt = gl_view_cvt.ImageToNDC(Vector2f(pt[0]+offset[0], pt[1]+offset[1]));

            glColor3fv(colour_ctrl_pt);
            glDrawCircle(pt[0], pt[1], gl_view_cvt.GetPixelScale());
        }
    }

    void DrawKnotsPts()
    {
        for(size_t k = 0; k < bspline.GetNumKnotPts(); ++k)
        {
            Vector2f pt = bspline.GetKnotPt(k);

            pt = gl_view_cvt.ImageToNDC(Vector2f(pt[0]+offset[0], pt[1]+offset[1]));

            glColor3fv(colour_knot_pt);
            glDrawCircle(pt[0], pt[1], gl_view_cvt.GetPixelScale());
        }
    }

    void DrawBspline()
    {
        for(int pt_idx = -1, seg_idx = 0; seg_idx <= bspline.GetNumCtrlPts(); ++pt_idx, ++seg_idx)
        {
            for(int d = 0; d < bspline.GetLOD(); ++d)
            {
                Vector2f pt1 = bspline.CubicIntplt(pt_idx, d/float(bspline.GetLOD()));
                Vector2f pt2 = bspline.CubicIntplt(pt_idx, (d+1)/float(bspline.GetLOD()));

                glColor3fv(colour_spline);
                glBegin(GL_LINES);
                glVertex(gl_view_cvt.ImageToNDC(Vector2f(pt1[0]+offset[0], pt1[1]+offset[1])));
                glVertex(gl_view_cvt.ImageToNDC(Vector2f(pt2[0]+offset[0], pt2[1]+offset[1])));
                glEnd();
            }
        }
    }

    void operator()(pangolin::View& view) {

        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);

        view.Activate();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        if(show_bspline) if(bspline.IsReady()) DrawBspline();
        if(show_ctrl_pts) DrawCtrlPts();
        if(show_knot_pts) DrawKnotsPts();

        glPopAttrib();
    }

private:

    GLViewCvt const& gl_view_cvt;

    Bspline<_Tp,dim> const& bspline;

    Vector2f offset;

    bool show_ctrl_pts;
    bool show_knot_pts;
    bool show_bspline;
};

#endif // LABEL_CATHETERE_PANGO_DRAWER
