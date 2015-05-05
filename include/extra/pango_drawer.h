#ifndef LABEL_CATHETERE_PANGO_DRAWER
#define LABEL_CATHETERE_PANGO_DRAWER

#include <queue>

#include <pangolin/pangolin.h>
#include <pangolin/gl.h>
#include <pangolin/glsl.h>

#include "../bspline.h"

using namespace Eigen;
using namespace pangolin;

/* Flip Y from the handler! */
class Handler2D : public Handler
{
public:
    Handler2D(size_t const w, size_t const h)
        : w(w), h(h)
    {
        has_picked_pt = false;
        has_selected_roi = false;

        selected_roi = Vector4f::Zero();
        picked_pt = Vector2f::Zero();
    }

    void WindowToImage(const Viewport& v, int wx, int wy, float& ix, float& iy) const
    {
        ix = w * (wx - v.l) /(float)v.w - 0.5;
        iy = h * (wy - v.b) /(float)v.h - 0.5;
        ix = std::max(0.0f,std::min(ix, w-1.0f));
        iy = h - std::max(0.0f,std::min(iy, h-1.0f)) - 1.0;
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
            WindowToImage(view.v, x, y, picked_pt[0], picked_pt[1]);
            has_picked_pt = true;
        }

    }

    virtual void MouseMotion(View& view, int x, int y, int /*button_state*/)
    {
        if(has_selected_roi)
            WindowToImage(view.v, x, y, selected_roi[2], selected_roi[3]);
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

    size_t w, h;

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
float colour_tip_pts[3] = {1.0, 0.0, 1.0};
float colour_tip_traj[3] = {1.0, 1.0, 0.0};

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
    DrawTexture(GlTexture const& tex)
        : tex(tex){}

    void operator()(pangolin::View& view) {

        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glColor4f(1.0, 1.0, 1.0, 1.0);

        view.Activate();

        tex.RenderToViewportFlipY();

        glPopAttrib();
    }

private:
    GlTexture const& tex;
};

template<typename _Tp, int dim>
class DrawBSpline
{
public:
    DrawBSpline(size_t const w, size_t const h, Bspline<_Tp,dim> const& bspline)
        : w(w), h(h), bspline(bspline), show_ctrl_pts(true), show_knot_pts(true), show_bspline(true)
    {}

    Vector2f ImageToNDC(Vector2f const img_pt) const {
        return Vector2f((img_pt[0]+0.5) * 2.0 / w - 1, ((h-img_pt[1])+0.5) * 2.0 / h - 1);
    }

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
            pt = ImageToNDC(Vector2f(pt[0], pt[1]));

            glColor3fv(colour_ctrl_pt);
            glDrawCircle(pt[0], pt[1], 0.005);
        }
    }

    void DrawKnotsPts()
    {
        for(size_t k = 0; k < bspline.GetNumKnotPts(); ++k)
        {
            Vector2f pt = bspline.GetKnotPt(k);

            pt = ImageToNDC(Vector2f(pt[0], pt[1]));

            glColor3fv(colour_knot_pt);
            glDrawCircle(pt[0], pt[1], 0.005);
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
                glVertex(ImageToNDC(Vector2f(pt1[0], pt1[1])));
                glVertex(ImageToNDC(Vector2f(pt2[0], pt2[1])));
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

    size_t w, h;

    Bspline<_Tp,dim> const& bspline;

    bool show_ctrl_pts;
    bool show_knot_pts;
    bool show_bspline;
};


typedef list<Vector2i> Pts;
typedef list<Pts> LabelData;

class DrawTip
{
public:
    DrawTip(size_t const w, size_t const h, LabelData const& label_data)
        : w(w), h(h), label_data(label_data), show_tip_traj(true)
    {}

    Vector2f ImageToNDC(Vector2f const img_pt) const {
        return Vector2f((img_pt[0]+0.5) * 2.0 / w - 1, ((h-img_pt[1])+0.5) * 2.0 / h - 1);
    }

    void ShowTipPts(bool const show) { show_tip_pts = show; }
    void ShowTipTraj(bool const show) { show_tip_traj = show; }

    void DrawTipPts() {

        for(auto label : label_data) {
            if(label.size() > 0) {
                Vector2f pt = ImageToNDC(Vector2f(label.back()[0], label.back()[1]));
                glColor3fv(colour_tip_pts);
                glDrawCircle(pt[0], pt[1], 0.002);
            }
        }

    }

    void DrawTipTraj() {

        queue<Vector2i> pts;

        for(auto label : label_data) {

            if(label.size() != 0) {

                pts.push(label.back());

                if(pts.size() == 2) {

                    glColor3fv(colour_tip_traj);
                    glBegin(GL_LINES);
                    glVertex(ImageToNDC(Vector2f(pts.front()[0], pts.front()[1])));
                    glVertex(ImageToNDC(Vector2f(pts.back()[0], pts.back()[1])));
                    glEnd();

                    pts.pop();
                }
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

        if(label_data.size() > 0) {

            if(show_tip_pts) DrawTipPts();
            if(show_tip_traj) DrawTipTraj();
        }

        glPopAttrib();
    }

private:

    size_t w, h;

    LabelData const& label_data;

    bool show_tip_pts;
    bool show_tip_traj;

};

#endif // LABEL_CATHETERE_PANGO_DRAWER
