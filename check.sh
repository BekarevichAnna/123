gcc hard.c -o hard.exe -fopenmp -O2 -D OMP_FOR_MODE="$2"
for i in {1..10}
do
    t=$(./hard.exe $1 test_data/in.pgm out.pgm | cut -d' ' -f 4)
    echo "$2;$t"
done