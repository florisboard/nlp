plugins {
    id("cpp-library")
}

group = "org.florisboard.nlp"
version = "0.1.0"

library {
    linkage.set(listOf(Linkage.STATIC))
}

tasks.withType<CppCompile> {
    compilerArgs.addAll("-std=c++2a")
}
