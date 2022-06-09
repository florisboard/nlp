plugins {
    id("cpp-application")
}

group = "org.florisboard.nlp"
version = "0.1.0"

application {
    dependencies {
        implementation("org.florisboard.nlp:core")
    }
}
