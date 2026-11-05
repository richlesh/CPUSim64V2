./clean.sh
rm lib/*
mvn clean
mvn package -DskipTests=true
cp ide/target/CPUSim64IDE-*.jar lib
cp cpusim64/target/cpusim64-*.jar lib
