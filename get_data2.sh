echo ";time" > "log2.csv"
for i in "static" "dynamic" "guided"
do
    for g in 1 2 4 8 16 32 64 128 256
    do
        ./check.sh 8 "$i, $g" | sed 's/\./,/'
    done
done >> "log2.csv"