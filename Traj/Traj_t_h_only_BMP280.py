import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


def calcul_trajectoire_t_h():
    df = pd.read_csv('donnees_fusee.csv')

    altitude = 44330 * (1 - (df['pressure'] / 1013.25) ** (1/5.257))
    t = np.array(df['time'])

    trajectory = np.zeros((len(t), 2))

    for i in range(len(trajectory) - 1):
        trajectory[i] = [altitude[i], t[i]]

    df_t = pd.DataFrame(trajectory, columns=['t', 'z'])
    df_t.to_csv('trajectoire_finale.csv', index=False)

    print(df_t)


def graphique_t_h():
    df_t = pd.read_csv('trajectoire_finale.csv')

    plt.plot(df_t['t'], df_t['z'])
    plt.xlabel('temps (s)')
    plt.ylabel('altitude (m)')

    plt.show()


def analyse():
    df = pd.read_csv('donnees_fusee.csv')

    altitude = 44330 * (1 - (df['pressure'] / 1013.25) ** (1/5.257))
    altitude_max = np.max(altitude)
    print(f"Altitude maximale : {altitude_max:.2f} m")


def main():
    calcul_trajectoire_t_h()
    analyse()
    graphique_t_h()


if __name__ == "__main__":
    main()