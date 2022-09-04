plugins {
    id("cpp-application")
}

group = "org.florisboard.nlp"
version = "0.1.0"

application {
    dependencies {
        implementation("io.github.bshoshany:thread-pool")
        implementation("org.florisboard.nlp:core")
    }
}

tasks.withType<CppCompile> {
    compilerArgs.addAll("-std=c++2a")//, "-pg")
}

tasks.withType<LinkExecutable> {
    linkerArgs.addAll("-pthread")//, "-pg")
}
