#ifndef LABEL_BSPLIN_BSPLINE_H
#define LABEL_BSPLIN_BSPLINE_H

#include <vector>

#include <Eigen/Core>
#include <Eigen/LU>

using namespace std;
using namespace Eigen;

//! @brief B-Spline template
template<typename _Tp,int dim>
class Bspline
{

public:
    enum BsplinType {OPEN, CLOSED} type;

    Bspline() : lod(30), type(OPEN)
    {
        CubicBsplineMatrix << 1.0, 4.0, 1.0, 0.0, -3.0, 0.0, 3.0, 0.0, 3.0, -6.0, 3.0, 0.0, -1.0, 3.0, -3.0, 1.0;
        CubicBsplineMatrix /= 6.0;
    }

    ~Bspline()
    {
        knot_pts.conservativeResize(NoChange, 0);
        ctrl_pts.conservativeResize(NoChange, 0);
    }

    void Clear()
    {
        knot_pts.conservativeResize(NoChange, 0);
        ctrl_pts.conservativeResize(NoChange, 0);
    }

    bool IsReady() const
    {
        return knot_pts.cols() > 3 && ctrl_pts.cols() > 3;
    }

    void AddKnotPt(Matrix<_Tp,dim,1> const& pt)
    {
        knot_pts.conservativeResize(NoChange, knot_pts.cols()+1);
        knot_pts.rightCols(1) = pt;
        CvtKnotToCtrlCubic();
    }

    void AddKnotPts(Matrix<_Tp,dim,Dynamic> const& pts)
    {
        knot_pts = pts;
        CvtKnotToCtrlCubic();
    }

    void AddCtrlPt(Matrix<_Tp,dim,1> const& pt)
    {
        ctrl_pts.conservativeResize(NoChange, ctrl_pts.cols()+1);
        ctrl_pts.rightCols(1) = pt;
        CvtCtrlToKnotCubic();
    }

    void AddCtrlPts(Matrix<_Tp,dim,Dynamic> const& pts)
    {
        ctrl_pts = pts;
        CvtCtrlToKnotCubic();
    }

    void SetKnotPt(size_t const p_idx, Matrix<_Tp,dim,Dynamic> const& pt)
    {
        knot_pts.col(GetPtIdx(p_idx)) = pt;
        CvtKnotToCtrlCubic();
    }

    void SetCtrlPt(size_t const p_idx, Matrix<_Tp,dim,Dynamic> const& pt)
    {
        ctrl_pts.col(GetPtIdx(p_idx)) = pt;
        CvtCtrlToKnotCubic();
    }

    Matrix<_Tp,dim,Dynamic> GetKnotPts() const
    {
        return knot_pts;
    }

    Matrix<_Tp,dim,1> GetKnotPt(size_t const p_idx) const
    {
        return knot_pts.col(GetPtIdx(p_idx));
    }

    Matrix<_Tp,dim,1> GetKnotFirstPt() const
    {
        return knot_pts.leftCols(1);
    }

    Matrix<_Tp,dim,1> GetKnotLastPt() const
    {
        return knot_pts.rightCols(1);
    }

    Matrix<_Tp,dim,Dynamic> GetCtrlPts() const
    {
        return ctrl_pts;
    }

    Matrix<_Tp,dim,1> GetCtrlPt(size_t const p_idx) const
    {
        return ctrl_pts.col(GetPtIdx(p_idx));
    }

    Matrix<_Tp,dim,1> GetCtrlFirstPt() const
    {
        return ctrl_pts.leftCols(1);
    }

    Matrix<_Tp,dim,1> GetCtrlLastPt() const
    {
        return ctrl_pts.rightCols(1);
    }

    size_t GetNumKnotPts() const { return knot_pts.cols(); }
    size_t GetNumCtrlPts() const { return ctrl_pts.cols(); }

    void SetBsplineType(BsplinType const type)
    {
        this->type = type;
        CvtKnotToCtrlCubic();
    }

    string GetBsplineType() const {
        switch(type)
        {
        case OPEN:
            return "Open B-spline";
        case CLOSED:
            return "Closed B-spline";
        }
    }

    _Tp GetLength() {

        _Tp arc_length = 0.0;

        if(IsReady()) {
            for(int pt_idx = -1, seg_idx = 0; seg_idx <= GetNumCtrlPts(); ++pt_idx, ++seg_idx) {
                for(int d = 0; d < lod; ++d) {
                    Matrix<_Tp,dim,1> pt1 = CubicIntplt(pt_idx, (d)/double(lod));
                    Matrix<_Tp,dim,1> pt2 = CubicIntplt(pt_idx, (d+1)/double(lod));

                    _Tp const dx = pt2[0] - pt1[0];
                    _Tp const dy = pt2[1] - pt1[1];

                    arc_length += sqrt(dx*dx + dy*dy);
                }
            }
        }

        return arc_length;
    }

    void KnotEquidist()
    {

        if(IsReady()) {

            bool has_converged = false;

            while(!has_converged) {

                _Tp const avg_arc_len = GetLength() / _Tp(GetNumKnotPts()-1);

                Matrix<_Tp,dim,Dynamic> new_knot_pts;
                new_knot_pts.conservativeResize(NoChange, 1);
                new_knot_pts.rightCols(1) = knot_pts.leftCols(1);

                _Tp begin_arc_len = 0.0;
                _Tp end_arc_len = 0.0;
                _Tp knot_arc_len = avg_arc_len;

                for(int pt_idx = -1, seg_idx = 0; seg_idx <= GetNumKnotPts(); ++pt_idx, ++seg_idx) {

                    begin_arc_len = end_arc_len;

                    for(int d = 0; d < lod; ++d) {

                        Matrix<_Tp,dim,1> pt1 = CubicIntplt(pt_idx, _Tp(d)/_Tp(lod));
                        Matrix<_Tp,dim,1> pt2 = CubicIntplt(pt_idx, _Tp(d+1)/_Tp(lod));

                        _Tp const dx = pt2[0] - pt1[0];
                        _Tp const dy = pt2[1] - pt1[1];

                        end_arc_len += sqrt(dx*dx + dy*dy);
                    }

                    while(end_arc_len - knot_arc_len > 1e-1) {

                        _Tp t = _Tp(knot_arc_len - begin_arc_len) / _Tp(end_arc_len - begin_arc_len);

                        new_knot_pts.conservativeResize(NoChange, new_knot_pts.cols()+1);

                        if(t > 1e-2)
                            new_knot_pts.rightCols(1) = CubicIntplt(pt_idx, t);
                        else
                            new_knot_pts.rightCols(1) = knot_pts.col(new_knot_pts.cols()-1);

                        knot_arc_len += avg_arc_len;
                    }
                }

                new_knot_pts.conservativeResize(NoChange, new_knot_pts.cols()+1);
                new_knot_pts.rightCols(1) = knot_pts.rightCols(1);

                if((new_knot_pts - knot_pts).norm() < 1e-1) {
                    knot_pts = new_knot_pts;
                    has_converged = true;
                }

                knot_pts = new_knot_pts;
                CvtKnotToCtrlCubic();
            }
        }

    }

    Matrix<_Tp,dim,1> CubicIntplt(int const pt_idx, _Tp const t, size_t const d_order = 0) const {

        if(GetNumCtrlPts() < 4)
            return Matrix<_Tp,dim,1>();

        if(t > 1.0 || t < 0) {
            cerr << "Curve parameter is not normalised!" << endl;
            return Matrix<_Tp,dim,1>();
        }

        /* Basis function coefficients */
        Matrix<_Tp,4,1> B;
        if(d_order == 0) {
            Matrix<_Tp,4,1> t_vec(1, t, t*t, t*t*t);
            B = t_vec.transpose()*CubicBsplineMatrix;
        } else if(d_order == 1) {
            Matrix<_Tp,4,1> t_vec(0, 1, 2*t, 3*t*t);
            B = t_vec.transpose()*CubicBsplineMatrix;
        } else if(d_order == 2) {
            Matrix<_Tp,4,1> t_vec(0, 0, 2, 6*t);
            B = t_vec.transpose()*CubicBsplineMatrix;
        } else if(d_order == 3) {
            Matrix<_Tp,4,1> t_vec(0, 0, 0, 6);
            B = t_vec.transpose()*CubicBsplineMatrix;
        }

        Matrix<_Tp,dim,1> pt;
        for(int i = 0; i < dim; ++i) {
            pt[i] = ctrl_pts(i, GetPtIdx(pt_idx-1))*B[0] +
                    ctrl_pts(i, GetPtIdx(pt_idx))*B[1] +
                    ctrl_pts(i, GetPtIdx(pt_idx+1))*B[2] +
                    ctrl_pts(i, GetPtIdx(pt_idx+2))*B[3];
        }

        return pt;
    }

    void SetLOD(size_t const lod) { this->lod = lod; }
    size_t GetLOD() const { return lod; }

    size_t GetPtIdx(int const pt_idx) const
    {
        size_t num_ctrl_pts = GetNumCtrlPts();
        size_t num_knot_pts = GetNumKnotPts();

        if(type == CLOSED)
        {
            /* Cycling */
            if(pt_idx < 0)
                return num_ctrl_pts+pt_idx;

            if(pt_idx >= num_ctrl_pts)
                return pt_idx-num_ctrl_pts;
        }
        else
        {
            /* Truncating */
            if(pt_idx < 0)
                return 0;

            if(pt_idx >= max(num_ctrl_pts, num_knot_pts))
                return max(num_ctrl_pts, num_knot_pts)-1;
        }

        return pt_idx;
    }

private:
    void CvtCtrlToKnotCubic()
    {
        size_t num_ctrl_pts = GetNumCtrlPts();

        if(num_ctrl_pts > 3)
        {

            knot_pts.conservativeResize(NoChange, num_ctrl_pts);
            knot_pts = Matrix<_Tp,dim,Dynamic>::Zero(dim, num_ctrl_pts);

            Matrix<_Tp,Dynamic,Dynamic> B = Matrix<_Tp,Dynamic,Dynamic>::Zero(num_ctrl_pts, num_ctrl_pts);

            if(type == OPEN)
            {
                B(0, 0) = 1.0;
                B(num_ctrl_pts-1, num_ctrl_pts-1) = 1.0;

                for(int c = 1; c < num_ctrl_pts-1; ++c)
                {
                    B(c, (c-1)%num_ctrl_pts) = 1.0/6.0;
                    B(c, (c)%num_ctrl_pts) = 2.0/3.0;
                    B(c, (c+1)%num_ctrl_pts) = 1.0/6.0;
                }
            }

            if(type == CLOSED)
            {
                for(int c = 0; c < num_ctrl_pts; ++c)
                {
                    B(c, c%num_ctrl_pts) = 1.0/6.0;
                    B(c, (c+1)%num_ctrl_pts) = 2.0/3.0;
                    B(c, (c+2)%num_ctrl_pts) = 1.0/6.0;
                }
            }

            knot_pts = (B*ctrl_pts.transpose()).transpose();

        }
    }

    void CvtKnotToCtrlCubic()
    {
        size_t num_knot_pts = GetNumKnotPts();
        if(num_knot_pts > 3)
        {
            ctrl_pts.conservativeResize(NoChange, num_knot_pts);
            ctrl_pts = Matrix<_Tp,dim,Dynamic>::Zero(dim, num_knot_pts);

            Matrix<_Tp,Dynamic,Dynamic> B = Matrix<_Tp,Dynamic,Dynamic>::Zero(num_knot_pts, num_knot_pts);
            Matrix<_Tp,Dynamic,Dynamic> inv_B;

            if(type == OPEN)
            {
                B(0, 0) = 1.0;
                B(num_knot_pts-1, num_knot_pts-1) = 1.0;

                for(int i = 1; i < num_knot_pts-1; ++i)
                {
                    B(i, (i-1)%num_knot_pts) = 1.0/6.0;
                    B(i, (i)%num_knot_pts) = 2.0/3.0;
                    B(i, (i+1)%num_knot_pts) = 1.0/6.0;
                }

                inv_B = B.inverse();
            }

            if(type == CLOSED)
            {
                for(int i = 0; i < num_knot_pts; ++i)
                {
                    B(i, i%num_knot_pts) = 1.0/6.0;
                    B(i, (i+1)%num_knot_pts) = 2.0/3.0;
                    B(i, (i+2)%num_knot_pts) = 1.0/6.0;
                }

                inv_B = B.inverse();
            }

            ctrl_pts = (inv_B*knot_pts.transpose()).transpose();

        }
    }

    /* Cubic Bspline basis matrix */
    Matrix<_Tp,4,4> CubicBsplineMatrix;

    /* Knot points */
    Matrix<_Tp,dim,Dynamic> knot_pts;

    /* Control points */
    Matrix<_Tp,dim,Dynamic> ctrl_pts;

    /* Level of details */
    size_t lod;

};

#endif // LABEL_BSPLIN_BSPLINE_H
