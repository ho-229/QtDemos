import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Particles 2.12

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Fireworks")

    maximumHeight: height
    minimumHeight: height
    maximumWidth: width
    minimumWidth: width

    Image {
        id: background
        source: "qrc:/image/background.png"
        anchors.fill: parent
        visible: true
    }

    ParticleSystem {
        id: fireworksSystem
        anchors.fill: parent

        Emitter {
            x: 274
            y: 358
            width: 314
            height: 35
            rotation: -25

            group: "launch"
            emitRate: 2
            lifeSpan: 1500
            size: 40
            endSize: 10

            maximumEmitted: 6

            velocity: AngleDirection {
                angle: 255
                angleVariation: 5

                magnitude: 250
                magnitudeVariation: 25
            }

            acceleration: PointDirection { y: 100 }
        }

        ImageParticle {
            id: fireball
            groups: "launch"
            source: "qrc:///particleresources/star.png"
            colorVariation: 1
        }

        ImageParticle {
            id: flame
            groups: ["flame", "burstFlame"]
            source: "qrc:///particleresources/glowdot.png"
        }

        TrailEmitter {
            id: fireballFlame
            anchors.fill: parent
            group: "flame"
            follow: "launch"

            emitRatePerParticle: 50
            lifeSpan: 400
            lifeSpanVariation: 50

            size: 10
            endSize: 3



            onEmitFollowParticles: {
                for(var i = 0; i < particles.length; i++) {
                    particles[i].red = followed.red;
                    particles[i].green = followed.green;
                    particles[i].blue = followed.blue;
                }
            }
        }

        ImageParticle {
            id: burstFirework
            groups: "burst"
            source: "qrc:///particleresources/star.png"
        }

        Emitter {
            id: burstEmitter
            group: "burst"
            lifeSpan: 2300
            lifeSpanVariation: 200

            size: 30
            endSize: 15

            enabled: false
            velocity: AngleDirection {
                angleVariation: 360
                magnitudeVariation: 60
            }
            acceleration: PointDirection { y: 15 }
        }

        TrailEmitter {
            group: "burstFlame"
            follow: "burst"
            anchors.fill: parent

            lifeSpan: 400

            emitRatePerParticle: 30
            size: 7
            endSize: 5

            onEmitFollowParticles: {
                for(var i = 0; i < particles.length; i++) {
                    particles[i].red = followed.red;
                    particles[i].green = followed.green;
                    particles[i].blue = followed.blue;
                    //flame.color = Qt.rgba(followed.red, followed.green, followed.blue, 1);  Linux use this
                }
            }
        }

        Affector {
            x: 17
            y: 46
            width: 607
            height: 192
            once: true
            groups: "launch"

            onAffectParticles: {
                for(const particle of particles) {
                    if(particle.lifeLeft() < 0.02) {
                        burstFirework.color = Qt.hsva(Math.random(), 1, 1, 1);
                        burstEmitter.burst(150, particle.x, particle.y);
                    }
                }
            }
        }
    }
}
