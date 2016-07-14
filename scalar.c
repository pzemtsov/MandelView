void calculate_float (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    float X0f = (float) X0;
    float Y0f = (float) Y0;
    float d = (float) scale;

    for (unsigned j = YSTART; j < SY; j++)	{
        float y0 = j*d + Y0f;
        for (unsigned i = 0; i < SX; i++)	{
            float x0 = i*d + X0f;
            float x = x0;
            float y = y0;
            unsigned n = 0;
            for (; n < 255; n++)	{
                float x2 = x * x;
                float y2 = y * y;
                if (x2 + y2 >= 4) break;
                y = 2 * x * y + y0;
                x = x2 - y2 + x0;
            }
            *out++ = n;
        }
    }
}

void calculate_double (unsigned char * out, double X0, double Y0, double scale, unsigned YSTART, unsigned SX, unsigned SY)
{
    for (unsigned j = YSTART; j < SY; j++)	{
        double y0 = j*scale + Y0;
        for (unsigned i = 0; i < SX; i++)	{
            double x0 = i*scale + X0;
            double x = x0;
            double y = y0;
            unsigned n = 0;
            for (; n < 255; n++)	{
                double x2 = x * x;
                double y2 = y * y;
                if (x2 + y2 >= 4) break;
                y = 2 * x * y + y0;
                x = x2 - y2 + x0;
            }
            *out++ = n;
        }
    }
}
