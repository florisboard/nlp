plugins {
    id("cpp-library")
}

group = "io.github.tessil"
version = "0.6.0"

library {
    publicHeaders.from(file("hat-trie/include"))
    linkage.set(listOf(Linkage.STATIC))
}

// Source of below config: https://github.com/gradle/gradle-native/issues/1074#issue-643161864
// Needed due to a lack of header-only library support by Gradle

components.withType<CppStaticLibrary>().configureEach {
    val outgoingArtifacts = linkElements.get().outgoing
    outgoingArtifacts.artifacts.removeIf {
        it.file == linkFile.get().asFile
    }
}

tasks.withType<CreateStaticLibrary>().configureEach {
    enabled = false
}

tasks.withType<CppCompile>().configureEach {
    enabled = false
}
