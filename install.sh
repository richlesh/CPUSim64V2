./clean.sh
rm lib/*
rm bin/*
mvn clean
mvn install -DskipTests=true
mkdir lib
cp ide/target/CPUSim64IDE-*.jar lib
cp cpusim64/target/cpusim64-*.jar lib
mkdir bin
cp cpusim64_native/target/release/cpusim64_native bin/
cp cpusim64_native/target/release/cpusim64_native bin/
cp cpusim64/src/main/resources/instructions.data bin/
