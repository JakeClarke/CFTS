cd ../
make
./aos-server
cd test/
rm aos-client
cp ../aos-client ./
gdb aos-client
pkill aos-server
