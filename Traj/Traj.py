import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

def calcul_trajectoire():
    df = pd.read_csv('donnees_fusee.csv')

    drift_ax = np.mean(df['accel_x'][:100])
    drift_ay = np.mean(df['accel_y'][:100])
    drift_az = np.mean(df['accel_z'][:100]) - 9.81 #si le MPU6050 est à l'envers, il faut ajouter 9.81

    drift_aRx = np.mean(df['accel_R_x'][:100])
    drift_aRy = np.mean(df['accel_R_y'][:100])
    drift_aRz = np.mean(df['accel_R_z'][:100])

    ax = np.array(df['accel_x']) - drift_ax
    ay = np.array(df['accel_y']) - drift_ay
    az = np.array(df['accel_z']) - drift_az

    aRx = np.array(df['accel_R_x']) - drift_aRx
    aRy = np.array(df['accel_R_y']) - drift_aRy
    aRz = np.array(df['accel_R_z']) - drift_aRz

    t = np.array(df['time'])

    vx = 0
    vy = 0
    vz = 0

    vRx = 0
    vRy = 0
    vRz = 0

    trajectory = np.zeros((len(t) + 1, 4))

    for i in range(len(t)):
        if i < len(t) - 1:
            dt = (t[i + 1] - t[i]) * 0.001

        vRx += aRx[i] * dt
        vRy += aRy[i] * dt
        vRz += aRz[i] * dt

        Rx = np.array([[1, 0, 0],
                    [0, np.cos(vRx), -np.sin(vRx)],
                    [0, np.sin(vRx), np.cos(vRx)]])
        
        Ry = np.array([[np.cos(vRy), 0, np.sin(vRy)],
                    [0, 1, 0],
                    [-np.sin(vRy), 0, np.cos(vRy)]])
        
        Rz = np.array([[np.cos(vRz), -np.sin(vRz), 0],
                    [np.sin(vRz), np.cos(vRz), 0],   
                    [0, 0, 1]])
        
        R = Rz @ Ry @ Rx

        ax_rel, ay_rel, az_rel = R @ np.array([ax[i], ay[i], az[i]]) - np.array([0, 0, 9.81])

        vx += ax_rel * dt
        vy += ay_rel * dt
        vz += az_rel * dt

        coordinates = np.array([1, vx, vy, vz]) * dt

        trajectory[i + 1] = coordinates + trajectory[i]

    df_t = pd.DataFrame(trajectory, columns=['t', 'x', 'y', 'z'])
    df_t.to_csv('trajectoire_finale.csv', index=False)

    print(df_t)


def graphique():
    df_t = pd.read_csv('trajectoire_finale.csv')

    fig = plt.figure()
    ax = plt.axes(projection='3d')
    ax.plot(df_t['x'], df_t['y'], df_t['z'])

    plt.show()


def main():
    calcul_trajectoire()
    graphique()


if __name__ == "__main__":
    main()