mvn clean
mvn package -DskipTests=true

./clean.sh
cp target/CPUSim64-2.0-SNAPSHOT.jar lib
