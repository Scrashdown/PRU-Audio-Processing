import numpy as np
import matplotlib.pyplot as plt
import sys

eps = sys.float_info.epsilon

def P_hat(N, M, R, fs):

    n_points = 1000
    freqs = np.linspace(0,(R / 2) * (1 / M),n_points)
    P_est = np.zeros((n_points,1))
    P = P_est

    for idx, f in enumerate(freqs):
        P_est[idx] = ( R*M * np.sin(np.pi*M*f) / (np.pi*M*f + eps) ) ** (2*N)
        P[idx] = (np.sin(np.pi*M*f)/(np.sin(np.pi*f/R)+eps))**(2*N)

    freqs_hz = freqs * fs / R

    # drop DC value since NaN due to division by 0
    return P[1:], P_est[1:], freqs_hz[1:]  

def bit_width(N, M, R):

    return np.ceil(1 + np.log2(R*M) * N)

if __name__ == "__main__":

    N = 4
    M = 1
    R = 16
    fs = 64 * 16e3
    fc = 8000      # target fs is 16kHz (speech)
    P, P_est, freqs_hz = P_hat(N,M,R,fs)
    B = bit_width(N, M, R)

    # convert to dB and normalize so we get attenuation
    P_db = 10*np.log10(np.abs(P))
    P_db = P_db - P_db.max()

    P_est_db = 10*np.log10(np.abs(P_est))
    P_est_db = P_est_db - P_est_db.max()

    # normalize according to CIC output frequency
    fr = fs / R

    # visualize
    plt.ion()
    plt.figure()
    plt.plot(freqs_hz / fr, P_db,'b-')
    plt.axvline(x=fc / fr, color='g', linestyle='--')

    # extract aliasing attenuation from first fold
    aliasing_atten_idx = np.argmin(abs(freqs_hz-fr+fc))
    aliasing_atten = P_est_db[aliasing_atten_idx][0]
    plt.plot(freqs_hz[aliasing_atten_idx]/fr,aliasing_atten,'rx')

    # plotting aliasing bandwidths
    for r in range(1, R//2):
        plt.axvline(x=r - fc / fr, color='r', linestyle='-')
        plt.axvline(x=r + fc / fr, color='r', linestyle='-')

    plt.tight_layout()
    plt.title('N=%d, M=%d, R=%d, B=%d'%(N,M,R,B), fontsize=20)
    plt.grid()
    plt.xlabel(('Frequency [Hz] / %dkHz')%(fr//1000), fontsize=20)
    plt.ylabel('Attenuation [dB]', fontsize=20)
    plt.xlim([0, 2.5])
    plt.ylim([-120,0])
    plt.legend(['Filter response', ('Cutoff=%dkHz')%(fc//1000), ('Aliasing attenuation=%ddB')%(int(aliasing_atten)), 'Aliasing bands'], fontsize=10)

    plt.tight_layout(pad=0.1)

    plt.show(block=True)