// swift-tools-version:4.0
import PackageDescription

let package = Package(
    name: "TPCircularBuffer",
    products: [
        .library(
            name: "TPCircularBuffer",
            targets: ["TPCircularBuffer"]),
    ],
    targets: [.target(name: "TPCircularBuffer")]
)
