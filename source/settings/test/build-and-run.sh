# remove test file to start fresh
rm settings.ini || true

# build and run the test
gcc -I .. -I ../../../mcuxpresso/ \
../../../mcuxpresso/minIni/minIni.c \
./settings_test.c \
&& ./a.out

# cleanup
rm ./a.out
rm settings.ini