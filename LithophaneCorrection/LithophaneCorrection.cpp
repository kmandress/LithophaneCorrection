#include <iostream>
#include <fstream>
#include <array>
#include <math.h>

using namespace std;

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include <windows.h>

cv::Mat generate_correction_lut(double min_thickness, double max_thickness, double fit_scale, double fit_exp);
double check_correction_lut(cv::Mat lut, double min_thickness, double max_thickness, double fit_scale, double fit_exp);

int main(int argc, char* argv[])
{
    if (!cv::haveImageReader(argv[1]))
    {
        fprintf(stderr, "error: can not read image type: %s\n", argv[1]);
        exit(-1);
    }

    cv::Mat input = cv::imread(argv[1]);
    if (input.empty())
    {
        fprintf(stderr, "error: can not read: %s\n", argv[1]);
        exit(-1);
    }

    double material_thickness = 0.02;
    double min_thickness = 0.02;
    double max_thickness = 0.24;

    double fit_scale = 0.0182;
    double fit_exp = 10.71;

    cv::Mat lut = generate_correction_lut(min_thickness, max_thickness, fit_scale, fit_exp);

    double rms = check_correction_lut(lut, min_thickness, max_thickness, fit_scale, fit_exp);

    printf("rms error: %f\n", rms);

    cv::Mat output(input.rows, input.cols, input.type());

    cv::LUT(input, lut, output);

    imwrite(argv[2], output);

	return 0;
}

cv::Mat generate_correction_lut(double min_thickness, double max_thickness, double fit_scale, double fit_exp)
{
    const int max_pixel = 255;

    cv::Mat desired_thickness(1, max_pixel + 1, CV_64F);
    double* dt_ptr = (double*)desired_thickness.ptr();
    cv::Mat partial_correction_lut(1, max_pixel+1, CV_64F);
    double* pcl_ptr = (double*)partial_correction_lut.ptr();

    for (int i = 0; i <= max_pixel; i++)
    {
        dt_ptr[i]  = max_thickness - i * (max_thickness - min_thickness) / (double)max_pixel;
        pcl_ptr[i] = log(fit_exp * dt_ptr[i]);
    }

    double a = (dt_ptr[max_pixel] - dt_ptr[0]) / (pcl_ptr[max_pixel] - pcl_ptr[0]);
    double b = dt_ptr[0] - a * pcl_ptr[0];

    //printf("a: %f\n", a);
    //printf("b: %f\n", b);    
    
    cv::Mat corrected_cut(1, max_pixel+1, CV_64F);
    double* cc_ptr = (double*)corrected_cut.ptr();
    cv::Mat output_lut(1, max_pixel + 1, CV_8U);
    uchar* ol_ptr = output_lut.ptr();

    for (int i = 0; i <= max_pixel; i++)
    {
        cc_ptr[i] = a * log(dt_ptr[i] * fit_exp) + b;
        ol_ptr[i] = (uchar)(max_pixel * cc_ptr[i] / max_thickness);
    }

    //for (int i = 0; i <= max_pixel; i++)
    //{
    //    printf("%d: %f %f %f %d\n", i, dt_ptr[i], pcl_ptr[i], cc_ptr[i], (int)ol_ptr[i]);
    //}

    return output_lut;
}

double check_correction_lut(cv::Mat lut, double min_thickness, double max_thickness, double fit_scale, double fit_exp)
{
    const int max_pixel = 255;

    uchar* lut_ptr = lut.ptr();

    double sum_sqr_error = 0;
    for (int i = 0; i <= max_pixel; i++)
    {
        double ideal_thickness = max_thickness - i * (max_thickness - min_thickness) / (double)max_pixel;
        double ideal_grey = max_pixel * ideal_thickness / max_thickness;
        double cut = max_thickness * lut_ptr[i] / max_pixel;
        double modeled = fit_scale * exp(cut * fit_exp);
        double grey = max_pixel * modeled / max_thickness;

        double error = ideal_grey - grey;
        double sqr_error = error * error;
        sum_sqr_error += sqr_error;

        //printf("%d: %f %f %f %f %f\n", i, cut, modeled, grey, ideal_grey, error);
    }

    return sqrt(sum_sqr_error / (max_pixel + 1));
}
