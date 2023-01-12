#include <stdio.h>
#include<stdlib.h>
#include <omp.h>
#include <string.h>
#ifdef _WIN32
    #include <sys\timeb.h> 
#else 
    #include <sys/time.h> 
#endif
typedef unsigned char byte;

#ifndef OMP_FOR_MODE
    #define OMP_FOR_MODE dynamic, 96
#endif
byte * image;
int width, height;
byte f0, f1, f2;
long long n;

long long histogram[256];
long long * histogram_calc[256];
double image_p[256];

double q_prefix[256];
double y_prefix[256];

int max_threads;
int thread_count;

void read_image(char * path){
    FILE * file = fopen(path, "rb");
    if(file == NULL){

    }
    fscanf(file, "P5\n%d %d\n255\n", &width, &height);
    n = ((long long)width) * height;
    image = (byte*)malloc(sizeof(byte) * n);
    fread(image, sizeof(byte), n, file);
    fclose(file);
}

void write_image(char * path){
    FILE * file = fopen(path, "wb");
    fprintf(file, "P5\n%d %d\n255\n", width, height);
    fwrite(image, sizeof(byte), n, file);
    free(image);
    fclose(file);
}

byte convert_pixel(byte v){
    if(v <= f0){
        return 0;
    }else if(v <= f1){
        return 84;
    }else if(v <= f2){
        return 170;
    }else{
        return 255;
    }
}

void convert_image(){
    #pragma omp parallel for schedule(OMP_FOR_MODE) if(thread_count != -1)
    for(long long i = 0; i < n;i++){
        image[i] = convert_pixel(image[i]);
    }
}

void caclulate_histogram(){
    #pragma omp parallel for schedule(OMP_FOR_MODE) if(thread_count != -1)
    for(long long i = 0; i < n;i++){
        #ifdef _OPENMP
            histogram_calc[image[i]][omp_get_thread_num()]++;
        #else
            histogram_calc[image[i]][0]++;
        #endif
    }
    #pragma omp parallel for schedule(OMP_FOR_MODE) if(thread_count != -1)
    for(int i = 0; i < 256;i++){
        for(int g = 0; g < max_threads;g++){
            histogram[i] += histogram_calc[i][g];
        }
    }
}

double calculate_q(int from, int to){
    return q_prefix[to] - (from == 0 ? 0 : q_prefix[from - 1]);
}

double calculate_y(int from, int to, double q){
    return (y_prefix[to] - (from == 0 ? 0 : y_prefix[from - 1])) / q;
}

double caclulate_dispersion(int p0, int p1, int p2){
    double q1 = calculate_q(0, p0);
    double q2 = calculate_q(p0 + 1, p1);
    double q3 = calculate_q(p1 + 1, p2);
    double q4 = calculate_q(p2 + 1, 255);

    double y1 = calculate_y(0, p0, q1);
    double y2 = calculate_y(p0 + 1, p1, q2);
    double y3 = calculate_y(p1 + 1, p2, q3);
    double y4 = calculate_y(p2 + 1, 255, q4);

    double y = q1 * y1 + q2 * y2 + q3 * y3 + q4 * y4;

    return q1 * (y1 - y)*(y1 - y) + q2 * (y2 - y)*(y2 - y) + q3 * (y3 - y)*(y3 - y) + q4 * (y4 - y)*(y4 - y);
}

void precalc(){
    for(int i = 0; i < 256;i++){
        image_p[i] = ((double)histogram[i]) / n;
        if(i == 0){
            q_prefix[i] = image_p[i];
            y_prefix[i] = i * image_p[i];
        }else{
            q_prefix[i] = q_prefix[i - 1] + image_p[i];
            y_prefix[i] =  y_prefix[i - 1] + i * image_p[i];
        }
    }
}
void calculate_f(){
    precalc();
    double mval[max_threads];
    for(int i =0; i < max_threads; i++) mval[i] = -1;
    int t0[max_threads], t1[max_threads], t2[max_threads];
    #pragma omp parallel for schedule(OMP_FOR_MODE) if(thread_count != -1)
    for(int p = 0; p < 256*256*256;p++){
        int p0 = p / (256 * 256);
        int p1 = (p / 256) % 256;
        int p2 = p % 256;
        if(p0 >= p1 || p1 >= p2){
            continue;
        }
        if(calculate_q(0, p0) == 0 || calculate_q(p0 + 1, p1) == 0 || calculate_q(p1 +1, p2) == 0 || calculate_q(p2 + 1, 255) == 0){
            continue;
        }
        double dispersion = caclulate_dispersion(p0, p1, p2);
        {
            int thread;
            #ifdef _OPENMP
                thread = omp_get_thread_num();
            #else
                thread = 0;
            #endif
            if(dispersion > mval[thread]){
                mval[thread] = dispersion;
                t0[thread] = p0;
                t1[thread] = p1;
                t2[thread] = p2;
             }
        }
    }
    int ans = 0;
    for(int i = 1; i < max_threads;i++){
        if(mval[i] > mval[ans]){
            ans = i;
        }
    }
    f0 = t0[ans];
    f1 = t1[ans];
    f2 = t2[ans];
}

int main(int argc, char ** argv){
    if(argc != 4){
        printf("Wrong arguments number");
        return 0;
    }
    
    sscanf(argv[1], "%d", &thread_count);
    #ifdef _OPENMP
        if(thread_count > 0){
            omp_set_num_threads(thread_count);
        }
        max_threads = omp_get_max_threads();
    #else
        max_threads = 1;
    #endif
    for(int i = 0; i < 256;i++){
        histogram_calc[i] = (long long*)malloc(sizeof(long long) * max_threads);
        memset(histogram_calc[i], 0ll, sizeof(long long) * max_threads);
    }
    read_image(argv[2]);
    double start;

    #ifdef _WIN32
        struct timeb start_t, end_t;
        ftime(&start_t);
        

    #else
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
    #endif

    #ifdef _OPENMP
        start = omp_get_wtime();
    #endif
    caclulate_histogram();
    calculate_f();
    
    convert_image();
    
    #ifdef _OPENMP
        printf("%d %d %d %g\n", f0, f1, f2, (omp_get_wtime() - start) * 1000);
    #else
        #ifdef _WIN32
            ftime(&end_t);
            double diff =  (1000.0 * (end_t.time - start_t.time)+ (end_t.millitm - start_t.millitm));
            printf("%d %d %d %lf\n", f0, f1, f2, diff);
        #else
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
            printf("%d %d %d %lf\n", f0, f1, f2, elapsedTime);
        #endif
        //printf("Time: %g ms\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - startChrono).count());
    #endif
    for(int i = 0; i < 256;i++){
        free(histogram_calc[i]);
    }
    write_image(argv[3]);
    // check input

}