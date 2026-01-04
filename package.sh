./clean.sh
rm lib/*
mvn clean
mvn package -DskipTests=true
cp target/CPUSim64*.jar lib
