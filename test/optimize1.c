// Tests register allocation with lots of variables
int main(int argc, char** argv) {
    int a = 10 + 1 * argc;
    int b = 20 + 2 * argc;
    int c = 30 + 3 * argc;
    int d = 40 + 4 * argc;
    int e = 50 + 5 * argc;
    int f = 60 + 6 * argc;
    int g = 70 + 7 * argc;
    int h = 80 + 8 * argc;
    int o = 90 + 9 * argc;
    int p = 100 + 10 * argc;
    int q = 110 + 11 * argc;
    int r = 120 + 12 * argc;
    int s = 130 + 13 * argc;
    int t = 140 + 14 * argc;
    int u = 150 + 15 * argc;
    int v = 160 + 16 * argc;
    int w = 170 + 17 * argc;
    int x = 180 + 18 * argc;
    int y = 190 + 19 * argc;
    int z = 200 + 20 * argc;

    for (int i = 0; i < c; ++i) {
        for (int j = 0; j < b; ++j) {
            for (int k = 0; k < a; ++k) {
                for (int l = 0; l < (c + b + a) / c; ++l) {
                    for (int m = 0; m < c / b * a / c * b / a * c / b; ++m) {
                        for (int n = 0; n < c + b - a + c - b + a - c + b - a + c - b + a; ++n) {
                            a = a - b + a / a + g + x;
                            b = e - f * a * b * d + e - y;
                            c = d / c + z;
                            d = a * f - a + h;
                            int h = f + c % b / c + c;
                            e = b * a + f - b / d + h;
                            f = d + d + c % h;
                            g = a + b - c * d / e + f - g * h / i + j - k * l / m + n - o * p / q + r - s * t / u + v - w * x / y + z;
                            h = z * y / x * w / v * u / t * s / r * q / p * o;
                            o = k * l / m + n - (a + b + c + d + e + f + g - h - i - j - k - l - m - n) * o * p * q;
                            p = a - b - c - d - e - f - g - h - i - j - k - l + o + p + q + r + s + t;
                            q = z + y + x + w + v + u + t + s + r + q + p + o + n + m - l - k - j - i - h - g - f - e - d - c - b - a;
                            r = 1 * 2 * 3 * 4 * 5 * 6 * 7 * 9 * 9 * 10 - 1 + 2 - 3 + 4 - 5;
                            s = t / u * v * w + x - y - z;
                            t = s - r * p + o / z * c + 1000 + a + b - c / 20 * d + e - f;
                            int a1 = a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p + q + r + s + t + u + v + w + x + y + z;
                            u = a1 + h - a + b - c + d + e - f + g - h + o - p + q - r + s - t + u - v + w - x + y - z;
                            v = a / b * c / d * e / f * g / h * i / j * k / l * m / n / o * p / q * r / s * t / u * v / w * x / y * z;
                            w = a % b * c % d % e * f % g * h % i * j % k * l % m * n % o * p % q * r % s * t % u * v % w * x % y * z;
                            y = x + w % v * t / p * o - h + g - d + h;
                            z = a - b - c - d - e - f - g - h - i - j - k - l - m - n - o - p - q - r - s - t - u - v - w - x - y - z;
                        }
                    }
                }
            }
        }
    }

    return (a / b * c - d + e - f + g + h + o * p - q - r + s / t / u + v + w + x * y - z) % 255;
}
