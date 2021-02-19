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
            x: 289
            y: 331
            width: 285
            height: 41

            group: "launch"
            emitRate: 1.25
            lifeSpan: 1500
            lifeSpanVariation: 50
            size: 40
            endSize: 10
            sizeVariation: 10

            velocity: AngleDirection {
                angle: 270
                angleVariation: 5

                magnitude: 250
                magnitudeVariation: 25
            }

            acceleration: PointDirection { y:100 }
        }

        ImageParticle {
            id: fireball
            groups: "launch"
            source: "qrc:///particleresources/star.png"
            //alphaVariation: 0.4
            colorVariation: 1
        }

        ImageParticle {
            groups: "flame"
            source: "qrc:///particleresources/glowdot.png"
            //colorVariation: 0.6
        }

        TrailEmitter {
            id: fireballFlame
            anchors.fill: parent
            group: "flame"
            follow: "launch"

            emitRatePerParticle: 100
            lifeSpan: 230
            lifeSpanVariation: 10
            //emitWidth: TrailEmitter.ParticleSize
            //emitHeight: TrailEmitter.ParticleSize
            //emitShape: EllipseShape{}

            size: 5
            //sizeVariation: 1
            endSize: 1

            onEmitFollowParticles: {
                for(var i = 0; i < particles.length; i++)
                {
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
            entryEffect: ImageParticle.Scale
            rotation: 60
            rotationVariation: 30
            rotationVelocity: 45
            rotationVelocityVariation: 15
        }

        Emitter {
            id: burstEmitter2
            group: "burst"
            emitRate: 15
            lifeSpan: 2000

            size: 25
            endSize: 5
            sizeVariation: 10

            enabled: false
            velocity: CumulativeDirection {
                AngleDirection {angleVariation: 360; magnitudeVariation: 80;}
                PointDirection {y: 20}
            }
            acceleration: PointDirection {y: 30 }
        }

        Affector {
            x: 0
            y: 30
            width: 640
            height: 135
            once: true
            groups: "launch"
            onAffectParticles: {
                for (var i = 0; i < particles.length; i++) {
                    if(particles[i].lifeLeft() < 0.02)
                    {
                        burstEmitter2.burst(300, particles[i].x, particles[i].y);
                        burstFirework.color = Qt.rgba(particles[i].red,
                                                      particles[i].green,
                                                      particles[i].blue,
                                                      particles[i].alpha);
                    }
                }
            }
         }
    }
}

