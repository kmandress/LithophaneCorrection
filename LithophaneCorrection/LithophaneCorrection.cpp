#include <iostream>
#include <fstream>
#include <array>
#include <math.h>

using namespace std;

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

#include <windows.h>

double check_correction_lut(cv::Mat lut, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_exp, int max_pixel);

uchar calculate_output(int pixel, int max_pixel, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_exp);

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

    const double material_thickness = 0.25;
    const double min_thickness = 0.02;
    const double max_thickness = 0.24;

    const double fit_scale = 0.0182;
    const double fit_exp = 10.71;

    const int max_pixel = 255;

    cv::Mat output_lut(1, max_pixel + 1, CV_8U);
    uchar* ol_ptr = output_lut.ptr();
    for (int i = 0; i <= max_pixel; i++)
    {
        ol_ptr[i] = calculate_output(i, max_pixel, min_thickness, max_thickness, material_thickness, fit_scale, fit_exp);
    }

    double rms = check_correction_lut(output_lut, min_thickness, max_thickness, material_thickness, fit_scale, fit_exp, max_pixel);

    printf("rms error: %f\n", rms);

    cv::Mat output(input.rows, input.cols, input.type());

    cv::LUT(input, output_lut, output);

    imwrite(argv[2], output);

	return 0;
}

double calc_greyscale_factor(double min_thickness, double material_thickness, int max_pixel)
{
    return (material_thickness - min_thickness) / (double)max_pixel;
}

uchar calc_greyscale(double thickness, double min_thickness, double material_thickness, int max_pixel)
{
    double greyscale_factor = calc_greyscale_factor(min_thickness, material_thickness, max_pixel);

    return (uchar)((material_thickness - thickness) / greyscale_factor + 0.5);
}

double calc_desired_thickness(int pixel, int max_pixel, double min_thickness, double max_thickness)
{
    return max_thickness - (pixel / (double)max_pixel) * (max_thickness - min_thickness);
}

double calc_partial_correction(int pixel, int max_pixel, double min_thickness, double max_thickness, double fit_exp)
{
    return log(fit_exp * calc_desired_thickness(pixel, max_pixel, min_thickness, max_thickness));
}

uchar calculate_output(int pixel, int max_pixel, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_exp)
{
    double desired_thickness_zero = calc_desired_thickness(0, max_pixel, min_thickness, max_thickness);
    double desired_thickness_max = calc_desired_thickness(max_pixel, max_pixel, min_thickness, max_thickness);
    double partial_correction_zero = calc_partial_correction(0, max_pixel, min_thickness, max_thickness, fit_exp);
    double partial_correction_max = calc_partial_correction(max_pixel, max_pixel, min_thickness, max_thickness, fit_exp);

    double a = (desired_thickness_max - desired_thickness_zero) / (partial_correction_max - partial_correction_zero);
    double b = desired_thickness_zero - a * partial_correction_zero;

    //printf("a: %f\n", a);
    //printf("b: %f\n", b);

    double desired_thickness = calc_desired_thickness(pixel, max_pixel, min_thickness, max_thickness);
    double corrected_thickness = a * log(desired_thickness * fit_exp) + b;
    uchar output = calc_greyscale(corrected_thickness, min_thickness, material_thickness, max_pixel);

    //printf("%d: %f %f %d\n", pixel, desired_thickness, corrected_thickness, (int)output);

    return output;
}

double check_correction_lut(cv::Mat lut, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_exp, int max_pixel)
{
    const double greyscale_factor = calc_greyscale_factor(min_thickness, material_thickness, max_pixel);

    uchar* lut_ptr = lut.ptr();

    //printf("\n");

    double sum_sqr_error = 0;
    for (int i = 0; i <= max_pixel; i++)
    {
        double desired_thickness = calc_desired_thickness(i, max_pixel, min_thickness, max_thickness);
        uchar ideal_grey = calc_greyscale(desired_thickness, min_thickness, material_thickness, max_pixel);

        double corrected_thickness = material_thickness - lut_ptr[i] * greyscale_factor;

        double scaled_modeled = fit_scale * exp(corrected_thickness * fit_exp);
        uchar scaled_output = calc_greyscale(scaled_modeled, min_thickness, material_thickness, max_pixel);

        int error = ideal_grey - scaled_output;
        int sqr_error = error * error;
        sum_sqr_error += sqr_error;

        //printf("%d: %f %d %d %f %f %d %d\n", i, desired_thickness, (int)ideal_grey, (int)lut_ptr[i], corrected_thickness, scaled_modeled, (int)scaled_output, error);
    }

    return sqrt(sum_sqr_error / ((double)max_pixel + 1));
}
