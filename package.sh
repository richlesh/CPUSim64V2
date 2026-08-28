./clean.sh
rm lib/*
mvn clean
mvn package -DskipTests=true
cp target/CPUSim64*.jar lib
cp ide/target/CPUSim64IDE-2.8.0.jar lib
cp cpusim64/target/cpusim64-2.8.0.jar lib
