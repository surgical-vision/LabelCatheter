#include <iomanip>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <pangolin/pangolin.h>
#include <pangolin/video.h>
#include <pangolin/timer.h>

#include <extra/pango_display.h>
#include <extra/pango_drawer.h>

#include <boost/gil/gil_all.hpp>
#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL
#include <boost/gil/extension/io/png_io.hpp>

using namespace boost::filesystem;

using namespace pangolin;
using namespace std;

void SetGLFormat(GLint& glchannels, GLenum& glformat, const pangolin::VideoPixelFormat& fmt)
{
    switch( fmt.channels) {
    case 1: glchannels = GL_LUMINANCE; break;
    case 3: glchannels = GL_RGB; break;
    case 4: glchannels = GL_RGBA; break;
    default: throw std::runtime_error("Unable to display video format");
    }

    switch (fmt.channel_bits[0]) {
    case 8: glformat = GL_UNSIGNED_BYTE; break;
    case 16: glformat = GL_UNSIGNED_SHORT; break;
    case 32: glformat = GL_FLOAT; break;
    default: throw std::runtime_error("Unknown channel format");
    }
}

void ExportViewport(string const img_name, Viewport const& viewport)
{

    boost::gil::rgb8_image_t img(viewport.w, viewport.h);
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(viewport.l, viewport.b, viewport.w, viewport.h, GL_RGB, GL_UNSIGNED_BYTE, boost::gil::interleaved_view_get_raw_data( boost::gil::view(img)));
    boost::gil::png_write_view(img_name, flipped_up_down_view(boost::gil::const_view(img)));

}

int main(int argc, char* argv[])
{

    if(argc < 2) {
        cerr << "Usage: " << argv[0] << " <video file>" << endl;
        exit(EXIT_FAILURE);
    }

    // Setup Video Source
    pangolin::VideoInput video(argv[1]);
    const pangolin::VideoPixelFormat vid_fmt = video.PixFormat();
    const unsigned w = video.Width();
    const unsigned h = video.Height();

    // Work out appropriate GL channel and format options
    GLint glchannels;
    GLenum glformat;
    SetGLFormat(glchannels, glformat, vid_fmt);

    uint32_t const ui_width = 180;
    pangolin::View& container = SetupPangoGL(w, h, ui_width, "Video Exporter");
    SetupContainer(container, 1, (float)w/h);

    pangolin::GlTexture frame_tex(w, h);
    DrawTexture tex_drawer(frame_tex);

    DrawingRoutine draw_routine;
    draw_routine.draw_funcs.push_back(std::ref(tex_drawer));

    container[0].SetDrawFunction(std::ref(draw_routine));

    Var<int> sample_rate("ui.Sample Rate", 10, 1, 100);
    Var<int> num_export_frames("ui.Frames to Export", 10000, 1, 10000);
    Var<int> frame_cur_idx("ui.Frame Idx");
    frame_cur_idx = 0;

    Var<bool> button_export_frames("ui.Export Frames", false, false);
    Var<bool> button_exist("ui.Exist", false, false);

    unsigned char* frame_buf = new unsigned char[video.SizeBytes()];
    if(video.GrabNext(frame_buf, true))
        frame_tex.Upload(frame_buf, glchannels, glformat);

    while(!pangolin::ShouldQuit())
    {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(Pushed(button_export_frames)) {

            /* Create output directory if not yet existing */
            ostringstream oss_output_dir;
            oss_output_dir << path(argv[1]).parent_path().string() << "/" <<
                              path(argv[1]).filename().replace_extension("").string() << "_PER" << (int)sample_rate << "F";
            if(!boost::filesystem::is_directory(path(oss_output_dir.str())))
                boost::filesystem::create_directories(path(oss_output_dir.str()));

            cout << "Export video images to " << oss_output_dir.str() << endl;

            /* Export the very fast frame */
            ostringstream oss_output_img;
            oss_output_img << oss_output_dir.str() << "/frame_" << setw(5) << setfill('0') << (int)frame_cur_idx << ".png";
            ExportViewport(oss_output_img.str(), container[0].v);

            frame_cur_idx = frame_cur_idx + sample_rate;
            int lock_sample_rate = sample_rate;

            while(video.GrabNext(frame_buf, true)) {

                for(int i = 0; i < sample_rate-1; ++i)
                    video.GrabNext(frame_buf, true);

                frame_tex.Upload(frame_buf, glchannels, glformat);
                pangolin::FinishFrame();

                oss_output_img.str("");
                oss_output_img << oss_output_dir.str() << "/frame_" << setw(5) << setfill('0') << (int)frame_cur_idx << ".png";
                ExportViewport(oss_output_img.str(), container[0].v);

                frame_cur_idx = frame_cur_idx + sample_rate;
                sample_rate = lock_sample_rate;

                if(Pushed(button_exist) || frame_cur_idx == num_export_frames)
                    break;
            }

            pangolin::Quit();
        }

        if(Pushed(button_exist))
            pangolin::Quit();

        // Swap frames and Process Events
        pangolin::FinishFrame();

    }

    delete [] frame_buf;
    return 0;

}
