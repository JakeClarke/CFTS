cd ../
make
./aos-server
cd test/
rm aos-client
cp ../aos-client ./
./aos-client -d
pkill aos-server
