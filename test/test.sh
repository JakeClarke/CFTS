cd ../
make
./aos-server
cd test/
rm aos-client
cp ../aos-client ./
./aos-client
pkill aos-server
