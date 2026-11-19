./clean.sh
mvn clean
mvn install -DskipTests=true
mkdir bin
cp ide/target/CPUSim64IDE-*.jar bin
cp cpusim64/target/cpusim64-*.jar bin
cp cpusim64_native/target/release/cpusim64_native bin/
cp cpusim64/src/main/resources/instructions.data bin/
