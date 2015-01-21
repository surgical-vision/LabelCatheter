#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <opencv2/highgui/highgui.hpp>

#include <pangolin/pangolin.h>
#include <pangolin/timer.h>

#include "include/extra/pango_display.h"
#include "include/extra/pango_drawer.h"
#include "include/bspline.h"

using namespace cv;
using namespace pangolin;
using namespace std;

////////////////////////////////////////////////////////////////////////////
//  Main function
////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{

    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <video file> <start frame number> <labelling frame gap>" << endl;
        exit(EXIT_FAILURE);
    }

    VideoCapture video_capturer(argv[1]);
    Mat frame;

    uint32_t w, h;
    if(video_capturer.isOpened()) {
        w = video_capturer.get(CV_CAP_PROP_FRAME_WIDTH);
        h = video_capturer.get(CV_CAP_PROP_FRAME_HEIGHT);
    } else  {
        cerr << "Couldn't open video file: " << argv[1] << endl;
    }

    uint32_t const ui_width = 180;
    pangolin::View& container = SetupPangoGL(w, h, ui_width, "Catheter Labelling");

    SetupContainer(container, 1, (float)w/h);

    GLViewCvt gl_view_cvt(w, h, true);
    Handler2D handler2d(gl_view_cvt);

    Bspline<float,2> bspline;
    DrawBSpline<float,2> bspline_drawer(gl_view_cvt, bspline);

    pangolin::GlTexture frame_tex(w, h);
    DrawTexture tex_drawer(gl_view_cvt, frame_tex);

    DrawingRoutine draw_routine;
    draw_routine.draw_funcs.push_back(std::ref(tex_drawer));
    draw_routine.draw_funcs.push_back(std::ref(bspline_drawer));

    container[0].SetDrawFunction(std::ref(draw_routine)).SetHandler(&handler2d);

    size_t basis_gap = 10;
    Var<int> label_frame_gap("ui.Label Frame Gap", 20, basis_gap, 500);
    label_frame_gap = (argc == 3 ? atoi(argv[2]) : label_frame_gap);

    Var<int> frame_end_idx("ui.Max Frame Idx");

    Var<int> frame_cur_idx("ui.Current Frame Idx");
    frame_cur_idx = -1;

    size_t frame_true_end_idx = video_capturer.get(CV_CAP_PROP_FRAME_COUNT);
    Var<int> frame_seek_idx("ui.Seek Frame Idx", 0, 0, frame_true_end_idx - basis_gap);

    Var<int> LevelOfDetail("ui.Level of Details", 10, 1, 50);
    Var<bool> check_show_bspline("ui.Show B-spline", true, true, false);
    Var<bool> check_show_knot_pts("ui.Show Knot Pts", true, true, false);
    Var<bool> check_show_ctrl_pts("ui.Show Ctrl Pts", true, true, false);

    Var<bool> button_reset("ui.Reset", false, false);
    Var<bool> button_export_label_img("ui.Export Label Image", false, false);

    // Register callback functions
    pangolin::RegisterKeyPressCallback(8, [&button_reset]() { button_reset = true; } );
    pangolin::RegisterKeyPressCallback(PANGO_SPECIAL + PANGO_KEY_LEFT, [&frame_seek_idx]() { frame_seek_idx = frame_seek_idx > 0 ? frame_seek_idx-1 : frame_seek_idx; } );
    pangolin::RegisterKeyPressCallback(PANGO_SPECIAL + PANGO_KEY_RIGHT, [&frame_seek_idx, &frame_end_idx]() { frame_seek_idx = frame_seek_idx < frame_end_idx ? frame_seek_idx+1 : frame_seek_idx; } );
    pangolin::RegisterKeyPressCallback(' ', [&button_export_label_img]() { button_export_label_img = true; });

    /* Create output directory */
    boost::filesystem::path current_path(boost::filesystem::current_path());
    string output_dir = current_path.string() + "/" + "output";
    if(!boost::filesystem::is_directory(boost::filesystem::path(output_dir)))
        boost::filesystem::create_directories(boost::filesystem::path(output_dir));

    while(!pangolin::ShouldQuit())
    {
        /* Binding frame seek and gap */
        label_frame_gap = (label_frame_gap/basis_gap)*basis_gap;
        frame_seek_idx = (frame_seek_idx/label_frame_gap)*label_frame_gap;
        frame_end_idx = (frame_true_end_idx/label_frame_gap)*label_frame_gap;

        bspline_drawer.ShowBspline(check_show_bspline);
        bspline_drawer.ShowCtrlPts(check_show_ctrl_pts);
        bspline_drawer.ShowKnotPts(check_show_knot_pts);

        if(Pushed(button_reset))
            bspline.Clear();

        if(Pushed(button_export_label_img)) {
            ostringstream oss_output_file_name;
            oss_output_file_name << output_dir << "/frame_" << setw(5) << setfill('0') << frame_cur_idx << ".png";
            imwrite(oss_output_file_name.str(), frame);

            Mat label_frame(frame.size(), CV_8UC4, Scalar(0, 0, 0, 255));
            for(int pt_idx = -1, seg_idx = 0; seg_idx <= bspline.GetNumCtrlPts(); ++pt_idx, ++seg_idx)
            {
                for(int d = 0; d < bspline.GetLOD(); ++d)
                {
                    Vector2f pt1 = bspline.CubicIntplt(pt_idx, d/float(bspline.GetLOD()));
                    Vector2f pt2 = bspline.CubicIntplt(pt_idx, (d+1)/float(bspline.GetLOD()));
                    line(label_frame, Point(pt1[0], pt1[1]), Point(pt2[0], pt2[1]), Scalar(255, 255, 255, 255), 1, 8);
                }
            }

            oss_output_file_name.str("");
            oss_output_file_name.clear();
            oss_output_file_name << output_dir << "/frame_" << setw(5) << setfill('0') << frame_cur_idx << "_label.png";

            cout << "Write: " << oss_output_file_name.str() << endl;

            imwrite(oss_output_file_name.str(), label_frame);

            bspline.Clear();

            /* Proceed the next */
            frame_seek_idx = frame_seek_idx + label_frame_gap;
            frame_seek_idx = frame_seek_idx > frame_end_idx ? frame_end_idx : frame_seek_idx;
        }

        /* Read new frame */
        if(frame_seek_idx != frame_cur_idx) {
            frame_cur_idx = frame_seek_idx;

            video_capturer.set(CV_CAP_PROP_POS_FRAMES, frame_cur_idx);
            video_capturer >> frame;

            frame_tex.Upload(frame.data, GL_BGR, GL_UNSIGNED_BYTE);
        }

        if(handler2d.HasPickedPt())
            bspline.AddKnotPt(handler2d.GetPickedPt());

        bspline.SetLOD(LevelOfDetail);

        container[0].ActivateScissorAndClear();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Swap frames and Process Events
        pangolin::FinishFrame();

    }

    return 0;
}
