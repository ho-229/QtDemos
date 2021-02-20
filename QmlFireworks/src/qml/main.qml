import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Particles 2.12

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Fireworks")

    color: "black";

    ParticleSystem {
        id: fireworksSystem
        anchors.fill: parent

        Emitter {
            x: 60
            y: 335
            width: 520
            height: 41

            group: "launch"
            emitRate: 2.25
            lifeSpan: 1500
            size: 40
            endSize: 10

            maximumEmitted: 6

            velocity: AngleDirection {
                angle: 270
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
            groups: ["flame", "burstFlame"]
            source: "qrc:///particleresources/glowdot.png"
        }

        TrailEmitter {
            id: fireballFlame
            anchors.fill: parent
            group: "flame"
            follow: "launch"

            emitRatePerParticle: 50
            lifeSpan: 230
            lifeSpanVariation: 10

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
            colorVariation: 0.2
        }

        Emitter {
            id: burstEmitter
            group: "burst"
            lifeSpan: 2300
            lifeSpanVariation: 200

            size: 30
            endSize: 15
            //sizeVariation: 10

            enabled: false
            velocity: AngleDirection {
                angleVariation: 360
                magnitudeVariation: 60
            }
            acceleration: PointDirection { y: 10 }
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
                }
            }
        }

        Age {
            x: 0
            y: 120
            width: 640
            height: 16
            once: true
            groups: "launch"

            onAffected: {
                burstEmitter.burst(90, x, y);
                burstFirework.color = Qt.hsva(Math.random(), 1, 1, 1);
            }
        }
    }
}
