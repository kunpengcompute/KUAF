#!/bin/bash
cd perftest/zlib_perf
make
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/kuaf/lib:$LD_LIBRARY_PATH

sudo sed -i '/\[KUAF_Section\]/,/^\[.*\]/ {
  s/^log_level=none$/log_level=debug/;
}' /etc/kuaf/kuaf.cnf
cat /etc/kuaf/kuaf.cnf

echo "bandwidth compress start"
./zip_perf -m 1 -t 1 -i -f ./itemdata -o itemdata.zlib
echo "bandwidth compress success"

sudo sed -i '/\[ZLIB-DEFLATE\]/,/^\[.*\]/ {
  s/^scheduler=bandwidth$/scheduler=ratio/;
  s/^bandwidth=.*$/ratio=60%/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "ratio compress start"
./zip_perf -m 20 -n 1 -i -f ./itemdata -o itemdata.zlib
echo "ratio compress success"

sudo sed -i '/\[ZLIB-DEFLATE\]/,/^\[.*\]/ {
  s/^scheduler=ratio$/scheduler=parallel/;
  s/^ratio=.*$/parallel=1/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "parallel compress start"
./zip_perf -m 20 -n 1 -i -f ./itemdata -o itemdata.zlib
echo "parallel compress success"

echo "ratio decompress start"
./zip_perf -m 20 -n 100 -i -f itemdata.zlib -d
echo "ratio decompress success"

sudo sed -i '/\[ZLIB-INFLATE\]/,/^\[.*\]/ {
  s/^scheduler=ratio$/scheduler=bandwidth/
  s/^ratio=.*$/bandwidth=1000/
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "bandwidth decompress start"
./zip_perf -m 20 -n 100 -i -f itemdata.zlib -d
echo "bandwidth decompress success"

sudo sed -i '/\[ZLIB-DEFLATE\]/,/^\[.*\]/ {
  s/^scheduler=bandwidth$/scheduler=parallel/;
  s/^bandwidth=.*$/parallel=1/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "parallel decompress start"
./zip_perf -m 20 -n 1 -i -f ./itemdata -o itemdata.zlib
echo "parallel decompress success"

cd ../../fuzztest
g++ ci_fuzz.cpp -I/usr/local/kuaf/include -L/usr/local/kuaf/lib -lkuaf -lkaezip -lz -lz_sw -o ci_fuzz
./ci_fuzz

cd functest/
make all
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/kuaf/lib:$LD_LIBRARY_PATH
./rsa_func
./sm2_func

cd ../perftest/crypto_perf
make all
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/kuaf/lib:$LD_LIBRARY_PATH

sudo sed -i '/\[KUAF_Section\]/,/^\[.*\]/ {
  s/^log_level=none$/log_level=debug/;
}' /etc/kuaf/kuaf.cnf
cat /etc/kuaf/kuaf.cnf

echo "bandwidth rsa start"
./rsa_perf
echo "bandwidth compress success"

sudo sed -i '/\[RSA\]/,/^\[.*\]/ {
  s/^scheduler=bandwidth$/scheduler=ratio/;
  s/^bandwidth=.*$/ratio=60%/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "ratio rsa start"
./rsa_perf
echo "ratio rsa success"

sudo sed -i '/\[RSA\]/,/^\[.*\]/ {
  s/^scheduler=ratio$/scheduler=parallel/;
  s/^ratio=.*$/parallel=1/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "parallel rsa start"
./rsa_perf
echo "parallel rsa success"

echo "bandwidth sm2 start"
./sm2_perf
echo "bandwidth sm2 success"

sudo sed -i '/\[SM2\]/,/^\[.*\]/ {
  s/^scheduler=bandwidth$/scheduler=ratio/;
  s/^bandwidth=.*$/ratio=60%/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "ratio sm2 start"
./sm2_perf
echo "ratio sm2 success"

sudo sed -i '/\[SM2\]/,/^\[.*\]/ {
  s/^scheduler=ratio$/scheduler=parallel/;
  s/^ratio=.*$/parallel=1/;
}' /etc/kuaf/kae.cnf
cat /etc/kuaf/kae.cnf

echo "parallel sm2 start"
./sm2_perf
echo "parallel sm2 success"

cd ../
sudo python kp_coverage.py