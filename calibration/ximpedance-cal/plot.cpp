#include <sciplot/sciplot.hpp>
using namespace sciplot;
#include <vector>

#include "voltage_to_current_10x_lut.h"

int main(int argc, char** argv) {
    Vec x = linspace(voltage_to_current_10x_lut_uv2ni[0][0],
                     voltage_to_current_10x_lut_uv2ni[413][0],
                     414);
    Plot2D plot;

    std::vector<float> data_x;
    std::vector<float> data_y;
    for (size_t i = 0; i < 414; i++) {
        data_x.push_back((float)voltage_to_current_10x_lut_uv2ni[i][0] * 1e-6);
        data_y.push_back((float)voltage_to_current_10x_lut_uv2ni[i][1] * 1e-9);
    }

    plot.drawCurve(data_x, data_y);

    Figure fig = {{ plot }};

    Canvas c = {{ fig }};

    c.show();
}
