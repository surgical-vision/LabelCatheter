#include <stdlib.h>
#include <time.h>

#include <iostream>
#include <algorithm>
#include <iomanip>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/gil/gil_all.hpp>
#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL
#include <boost/gil/extension/io/png_io.hpp>

#include <pangolin/pangolin.h>
#include <pangolin/video.h>
#include <pangolin/timer.h>

#include <extra/pango_display.h>
#include <extra/pango_drawer.h>
#include <bspline.h>

using namespace boost::filesystem;
using namespace boost::gil;

using namespace pangolin;
using namespace std;

int ParseCSVFile(string const& csv_file)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(csv_file.c_str(), "r");
    if(fp == NULL)
        return 0;

    int label_line_num = -1; // First line is the field names
    while ((read = getline(&line, &len, fp)) != -1)
        label_line_num++;

    fclose(fp);

    if(line)
        free(line);

    return label_line_num;

}

////////////////////////////////////////////////////////////////////////////
//  Main function
////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{

    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <images dir>" << endl;
        exit(EXIT_FAILURE);
    }

    string dir = string(argv[1]);

    /* Import all frames for labelling */
    vector<string> img_files;
    directory_iterator end_itr;
    try {
    for (directory_iterator itr(dir); itr != end_itr; ++itr)
        if (is_regular_file(itr->path()))
            if(itr->path().extension() == ".png" && itr->path().string().find("frame") != string::npos)
                img_files.push_back(itr->path().filename().string());
    } catch(filesystem_error& e) {
        cerr << e.code().message() << ": " << dir << endl;
        return 1;
    }

    /* Parse existing CSV file */
    int existing_label_idx = ParseCSVFile(dir + "/" + "label.csv");

    if(existing_label_idx == img_files.size()) {
        cout << "All images have been labelled!" << endl;
        return 0;
    }

    /* Read image */
    rgb8_image_t img;
    png_read_image(dir + "/" + img_files[existing_label_idx], img);

    // Setup Video Source    
    const unsigned w = img.width();
    const unsigned h = img.height();

    uint32_t const ui_width = 180;
    pangolin::View& container = SetupPangoGL(w, h, ui_width, "Video Exporter");
    SetupContainer(container, 1, (float)w/h);

    GLViewCvt gl_view_cvt(w, h, true);
    Handler2D handler2d(gl_view_cvt);

    Bspline<float,2> bspline;
    DrawBSpline<float,2> bspline_drawer(gl_view_cvt, bspline);

    pangolin::GlTexture img_tex(w, h, GL_RGBA, true, 0, GL_RGB, GL_UNSIGNED_BYTE, interleaved_view_get_raw_data(view(img)));
    DrawTexture tex_drawer(gl_view_cvt, img_tex);

    DrawingRoutine draw_routine;
    draw_routine.draw_funcs.push_back(std::ref(tex_drawer));
    draw_routine.draw_funcs.push_back(std::ref(bspline_drawer));

    container[0].SetDrawFunction(std::ref(draw_routine)).SetHandler(&handler2d);

    Var<int> totle_img("ui.Totle Images");
    totle_img = img_files.size();

    Var<int> img_cur_idx("ui.Image Current Idx");
    img_cur_idx = existing_label_idx+1;

    Var<bool> check_show_bspline("ui.Show B-spline", true, true, false);
    Var<bool> check_show_knot_pts("ui.Show Knot Pts", true, true, false);
    Var<bool> check_show_ctrl_pts("ui.Show Ctrl Pts", false, true, false);

    Var<bool> button_reset("ui.Reset", false, false);
    Var<bool> button_export_label("ui.Export Label", false, false);

    // Register callback functions
    pangolin::RegisterKeyPressCallback(8, [&button_reset]() { button_reset = true; } );
    pangolin::RegisterKeyPressCallback(' ', [&button_export_label]() { button_export_label = true; });

    while(!pangolin::ShouldQuit())
    {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(handler2d.HasPickedPt())
            bspline.AddKnotPt(handler2d.GetPickedPt());

        bspline_drawer.ShowBspline(check_show_bspline);
        bspline_drawer.ShowCtrlPts(check_show_ctrl_pts);
        bspline_drawer.ShowKnotPts(check_show_knot_pts);

        if(Pushed(button_reset))
            bspline.Clear();

        if(Pushed(button_export_label)) {            

            /* Export label image */
            gray8_image_t label_img(w, h);
            fill_pixels(view(label_img), 0);

            vector<Vector2i> const& body_pts = bspline.GetContinuousPts();
            for(auto pt : body_pts)
                view(label_img)(pt[0], pt[1]) = 255;

            string label_img_file = img_files[(int)img_cur_idx-1];
            label_img_file.replace(0, 5, "label");

            cout << "Write: " << dir << "/" << label_img_file << endl;
            boost::gil::png_write_view(dir + "/" + label_img_file, const_view(label_img));

            /* Export CSV file */            
            string csv_file = "label.csv";
            if(!exists((dir + "/" + csv_file))) {
                FILE* fout = fopen((dir + "/" + csv_file).c_str(), "w");
                fprintf(fout, "frame idx,\ttip_xy,\tbase_xy,\tnum_body_pt,\tbody_xy");
                fprintf(fout, "\n");
                fclose(fout);
            }

            string cur_frame_idx = label_img_file.substr(label_img_file.find("_")+1, label_img_file.find(".")-label_img_file.find("_")-1);
            FILE* fout = fopen((dir + "/" + csv_file).c_str(), "a");

            if(body_pts.size() > 0) {

                fprintf(fout, "%d", atoi(cur_frame_idx.c_str())); // Frame No.

                fprintf(fout, ",\t%d %d", body_pts.back()[0], body_pts.back()[0]); // Tip point
                fprintf(fout, ",\t%d %d", body_pts.front()[0], body_pts.front()[1]); // Base point

                fprintf(fout, ",\t%d", body_pts.size()); // Num of body points

                fprintf(fout, ",\t"); // Body point
                for(int i = 0; i < body_pts.size(); ++i)
                    fprintf(fout, "%d %d ", body_pts[i][0], body_pts[i][1]);

                fprintf(fout, "\n");
            } else {

                fprintf(fout, "%d", atoi(cur_frame_idx.c_str())); // Frame No.

                fprintf(fout, ",\t"); // Tip point
                fprintf(fout, ",\t"); // Base point

                fprintf(fout, ",\t"); // Num of body points

                fprintf(fout, ",\t"); // Body point

                fprintf(fout, "\n");

            }

            fclose(fout);

            /* Proceed the next */
            if(img_cur_idx == img_files.size()) {
                cout << "All done!" << endl;
                pangolin::Quit();
            }
            else {
                img_cur_idx = img_cur_idx + 1;
                png_read_image(dir + "/" + img_files[(int)img_cur_idx-1], img);
                img_tex.Upload(interleaved_view_get_raw_data(view(img)), GL_RGB, GL_UNSIGNED_BYTE);
            }

            bspline.Clear();
        }

        container[0].ActivateScissorAndClear();
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // Swap frames and Process Events
        pangolin::FinishFrame();

    }

    return 0;
}
