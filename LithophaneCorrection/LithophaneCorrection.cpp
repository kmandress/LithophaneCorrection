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
    const double min_thickness      = 0.02;
    const double max_thickness      = 0.24;

    const double fit_scale  = 0.0844;
    const double fit_offset = 0.3675;

    const int max_pixel = 255;

    cv::Mat output_lut(1, max_pixel + 1, CV_8U);
    uchar* ol_ptr = output_lut.ptr();
    for (int i = 0; i <= max_pixel; i++)
    {
        ol_ptr[i] = calculate_output(i, max_pixel, min_thickness, max_thickness, material_thickness, fit_scale, fit_offset);
    }

    double rms = check_correction_lut(output_lut, min_thickness, max_thickness, material_thickness, fit_scale, fit_offset, max_pixel);

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

double calc_greyscale(double thickness, double min_thickness, double material_thickness, int max_pixel)
{
    double greyscale_factor = calc_greyscale_factor(min_thickness, material_thickness, max_pixel);

    return (material_thickness - thickness) / greyscale_factor;
}

double calc_desired_thickness(int pixel, int max_pixel, double min_thickness, double max_thickness)
{
    return max_thickness - (pixel / (double)max_pixel) * (max_thickness - min_thickness);
}

double calc_partial_correction(int pixel, int max_pixel, double min_thickness, double max_thickness, double fit_exp)
{
    return log(fit_exp * calc_desired_thickness(pixel, max_pixel, min_thickness, max_thickness));
}

uchar calculate_output(int pixel, int max_pixel, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_offset)
{
    double desired_thickness = calc_desired_thickness(pixel, max_pixel, min_thickness, max_thickness);
    double raw_corrected_thickness = exp((desired_thickness - fit_offset) / fit_scale);

    double max_unconstrained_pixel = calc_greyscale(exp((min_thickness - fit_offset) / fit_scale), min_thickness, material_thickness, max_pixel);
    uchar output = (uchar)round(calc_greyscale(raw_corrected_thickness, min_thickness, material_thickness, 2 * max_pixel - (int)round(max_unconstrained_pixel)));

    printf("%d: %f %f %d\n", pixel, desired_thickness, raw_corrected_thickness, (int)output);

    return output;
}

double check_correction_lut(cv::Mat lut, double min_thickness, double max_thickness, double material_thickness, double fit_scale, double fit_offset, int max_pixel)
{
    const double greyscale_factor = calc_greyscale_factor(min_thickness, material_thickness, max_pixel);

    uchar* lut_ptr = lut.ptr();

    printf("\n");

    double sum_sqr_error = 0;
    for (int i = 0; i <= max_pixel; i++)
    {
        double desired_thickness = calc_desired_thickness(i, max_pixel, min_thickness, max_thickness);
        uchar ideal_grey = calc_greyscale(desired_thickness, min_thickness, material_thickness, max_pixel);

        double corrected_thickness = material_thickness - lut_ptr[i] * greyscale_factor;

        double optical_corrected_thickness = fit_scale * log(corrected_thickness) + fit_offset;
        uchar equivelent_output = round(calc_greyscale(optical_corrected_thickness, min_thickness, material_thickness, max_pixel));

        double error = desired_thickness - optical_corrected_thickness;
        double sqr_error = error * error;
        sum_sqr_error += sqr_error;

        printf("%d: %f %d %d %f %f %d %f\n", i, desired_thickness, (int)ideal_grey, (int)lut_ptr[i], corrected_thickness, optical_corrected_thickness, (int)equivelent_output, error);
    }

    return sqrt(sum_sqr_error / ((double)max_pixel + 1));
}
