import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    kotlin("jvm") version "1.3.72"
    application
}

group = "com.android"
version = "1.0.0-SNAPSHOT"

repositories {
    mavenCentral()
    jcenter()
    google()
    maven(url = "https://dl.bintray.com/s1m0nw1/KtsRunner")
}

dependencies {
    implementation(kotlin("stdlib", "1.3.72"))
    implementation(kotlin("reflect", "1.3.72"))

    implementation("com.google.prefab:api:1.0.0")

    implementation("com.github.ajalt:clikt:2.2.0")
    implementation("de.swirtz:ktsRunner:0.0.7")
    implementation("org.apache.maven:maven-core:3.6.2")
    implementation("org.redundent:kotlin-xml-builder:1.5.3")

    testImplementation("org.jetbrains.kotlin:kotlin-test")
    testImplementation("org.jetbrains.kotlin:kotlin-test-junit")
    testImplementation("org.junit.jupiter:junit-jupiter-api:5.6.0-M1")
    testRuntimeOnly("org.junit.jupiter:junit-jupiter-engine:5.6.0-M1")
}

application {
    // Define the main class for the application.
    mainClassName = "com.android.ndkports.CliKt"
}

tasks.withType<KotlinCompile> {
    kotlinOptions.jvmTarget = "1.8"
    kotlinOptions.freeCompilerArgs += listOf(
        "-progressive",
        "-Xuse-experimental=kotlinx.serialization.ImplicitReflectionSerializer"
    )
}

val portsBuildDir = buildDir.resolve("ports")

val allPorts = listOf("openssl", "curl", "jsoncpp")

// Can be specified in ~/.gradle/gradle.properties:
//
//     ndkPath=/path/to/ndk
//
// Or on the command line:
//
//     ./gradlew -PndkPath=/path/to/ndk run
val ndkPath: String by project
tasks.named<JavaExec>("run") {
    // Order matters since we don't do any dependency sorting, so we can't just
    // use the directory list.
    args = listOf("--ndk", ndkPath, "-o", portsBuildDir.toString()) + allPorts
}

for (port in allPorts) {
    distributions {
        create(port) {
            contents {
                includeEmptyDirs = false
                from(portsBuildDir.resolve(port)) {
                    include("**/*.aar")
                    include("**/*.pom")
                }
            }
        }
    }

    tasks.named("${port}DistTar") {
        dependsOn(":run")
    }

    tasks.named("${port}DistZip") {
        dependsOn(":run")
    }
}

distributions {
    create("all") {
        contents {
            includeEmptyDirs = false
            from(portsBuildDir) {
                include("**/*.aar")
                include("**/*.pom")
            }
        }
    }
}

tasks.named("allDistTar") {
    dependsOn(":run")
}

tasks.named("allDistZip") {
    dependsOn(":run")
}

tasks.register("release") {
    dependsOn(":allDistZip")
    for (port in allPorts) {
        dependsOn(":${port}DistZip")
    }
}
