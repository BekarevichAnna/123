echo ";time" > log3.csv
gcc hard.c -o hard.exe -fopenmp -O2
for i in {1..10}
do
    t=$(./hard.exe -1 test_data/in.pgm out.pgm | cut -d' ' -f 4 | sed 's/\./,/')
    echo "-1;$t"
done >> log3.csv
gcc hard.c -o hard.exe -O2
for i in {1..10}
do
    t=$(./hard.exe -1 test_data/in.pgm out.pgm | cut -d' ' -f 4 | sed 's/\./,/')
    echo "no fopenmp;$t"
done >> log3.csv