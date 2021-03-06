import argparse
import joblib
import matplotlib.pyplot as plt
import numpy as np
import os
import pandas as pd
import warnings

from prepare_calibration_data import import_train_data
from prepare_tpch import import_test_data
from sklearn.ensemble import GradientBoostingRegressor
from sklearn.linear_model import Lasso
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge
from sklearn.metrics import mean_squared_error
from sklearn.model_selection import train_test_split
plt.style.use('ggplot')

def preprocess_data(data):
    # one-hot encoding
    ohe_data = data.drop(labels=['TABLE_NAME', 'COLUMN_NAME'], axis=1)
    ohe_data = pd.get_dummies(ohe_data, columns=['SCAN_TYPE', 'DATA_TYPE', 'ENCODING', 'SCAN_IMPLEMENTATION', 'COMPRESSION_TYPE'])
    return ohe_data


def train_model(train_data, type):
    ohe_data = preprocess_data(train_data)
    y = np.ravel(ohe_data[['RUNTIME_NS']])
    X = ohe_data.drop(labels=['RUNTIME_NS'], axis=1)
    if type == 'linear':
        model = LinearRegression().fit(X, y)
    elif type == 'ridge':
        model = Ridge(alpha=10).fit(X, y)
    elif type == 'lasso':
        model = Lasso(alpha=10).fit(X, y)
    elif type == 'boost':
        model = GradientBoostingRegressor(loss='huber').fit(X, y)

    return model


def generate_model_plot(model, test_data, method, encoding, scan, out):
    ohe_data = preprocess_data(test_data)
    real_y = np.ravel(ohe_data[['RUNTIME_NS']])
    ohe_data = ohe_data.drop(labels=['RUNTIME_NS'], axis=1)
    pred_y = model.predict(ohe_data)

    model_scores = calculate_error(ohe_data, pred_y, real_y, model)

    plt.scatter(real_y, pred_y, c='orange')

    pred_y = model.predict(ohe_data)
    axis_max = max(np.amax(pred_y), np.amax(real_y)) * 1.05
    axis_min = min(np.amin(pred_y), np.amin(real_y)) * 0.95
    abline_values = range(int(axis_min), int(axis_max), int((axis_max-axis_min)/100))

    # Plot the best fit line over the actual values
    plt.plot(abline_values, abline_values, c = 'r', linestyle="-")
    plt.title(f"{encoding}_{scan}_{method}; Score: {model_scores['R2']}")
    plt.ylim([axis_min, axis_max])
    plt.xlim([axis_min, axis_max])
    plt.xlabel("Real Time")
    plt.ylabel("Predicted Time")
    output_path = f'{out}/plots/{method}_{encoding}_{scan}'
    plt.savefig(output_path, bbox_inches='tight')
    plt.close()

    return model_scores


def calculate_error(test_X, y_true, y_pred, model):
    # calculate Root-mean-squared error (RMSE) for the model
    RMSE = mean_squared_error(y_true, y_pred, squared=False)

    # The score function returns the coefficient of determination R^2 of the prediction.
    # It gives a fast notion of how well a model is predicting since the score can be between -1 and 1; 1 being the
    # optimum and 0 meaning the model is as good as just predicting the median of the training data.
    R2 = model.score(test_X, y_pred)

    # logarithm of the accuracy ratio (LnQ)
    LNQ = 1/len(y_true) * np.sum(np.exp(np.divide(y_pred, y_true)))

    # mean absolute percentage error (MAPE)
    MAPE = np.mean(100 * (np.divide(np.abs(y_true - y_pred), y_true)))

    scores = {'RMSE': '%.3f' % RMSE, 'R2': '%.3f' % R2, 'LNQ': '%.3f' % LNQ, 'MAPE': '%.3f' % MAPE}

    return scores


def log(scores, out):
    with open(f'{out}/log.txt', 'w') as file:
        for entry in scores:
            file.write(f"{entry}: {scores[entry]} \n")


# needed for prediction with one-hot-encoding in case trainings and test data don't have the same set of values in a
# categorical data column
def add_dummy_types(train, test, cols):
    for col in cols:
        diff1 = np.setdiff1d(train[col].unique(), test[col].unique())
        diff2 = np.setdiff1d(test[col].unique(), train[col].unique())
        for d1 in diff1:
            test[f'{col}_{d1}'] = 0
        for d2 in diff2:
            train[f'{col}_{d2}']= 0
    return [train, test]


def parse_arguments(opt=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('-train', help='Trainingsdata in csv format', action='append', nargs='+')
    # in case no test data is given, the trainings data will be split into trainings and test data
    parser.add_argument('--test', help='Testdata in csv format', action='append', nargs='+')
    parser.add_argument('--m', help='Model type: choose from "linear", "lasso", "ridge", "boost"; Boost is the default"')
    parser.add_argument('--out', help='Output folder')

    if (opt):
        return parser.parse_args(opt)
    else:
        return parser.parse_args()


def import_data(args):
    test_data = import_train_data(args.test[0])
    test_data = test_data.dropna()

    train_data = import_train_data(args.train[0])
    train_data = train_data.dropna()

    # check whether trainings and testdata have the same format
    if test_data.columns.all() != train_data.columns.all():
        warnings.warn("Warning: Trainings- and Testdata do not have the same format. Unmatched columns will be ignored!")

    inter = train_data.columns.intersection(test_data.columns)
    test_data = test_data[inter.tolist()]
    train_data = train_data[inter.tolist()]

    return [train_data, test_data]


def main(args):
    out = "costModelOutput"
    scores = {}

    if args.m:
        model_types = [args.m]
    else:
        # We have achieved the best results with this model
        model_types = ['boost']

    if args.test:
        train_data, test_data = import_data(args)
    else:
        train_data = import_train_data(args.train[0])
        train_data = train_data.dropna()
        train_data, test_data = train_test_split(train_data)

    if args.out:
        out = args.out

    if not os.path.exists(f"{out}/models"):
        os.makedirs(f"{out}/models")

    if not os.path.exists(f"{out}/plots"):
        os.makedirs(f"{out}/plots")

    # one single model for everything
    for type in model_types:
        gtrain_data, gtest_data = add_dummy_types(train_data.copy(), test_data.copy(), ['COMPRESSION_TYPE', 'SCAN_IMPLEMENTATION', 'SCAN_TYPE', 'DATA_TYPE', 'ENCODING'])
        gmodel = train_model(gtrain_data, type)
        scores[f'{type}_general_model'] = generate_model_plot(gmodel, gtest_data, type, 'all', 'all',out)
        filename = f'{out}/models/{type}_general_model.sav'
        joblib.dump(gmodel, filename)

    # make separate models for different scan operators and combinations of encodings/compressions
    for encoding in train_data['ENCODING'].unique():
        for implementation_type in train_data['SCAN_IMPLEMENTATION'].unique():
            try:
                # if there is no given test data set, split the given trainings data into test and trainings data
                if not args.test:
                        model_train_data, model_test_data = train_test_split(train_data.loc[(train_data['ENCODING'] == encoding) &
                                                                                            (train_data['SCAN_IMPLEMENTATION'] == implementation_type)])

                else:
                    model_train_data = train_data.loc[(train_data['ENCODING'] == encoding) & (train_data['SCAN_IMPLEMENTATION'] == implementation_type)]
                    model_test_data = test_data.loc[(test_data['ENCODING'] == encoding) & (test_data['SCAN_IMPLEMENTATION'] == implementation_type)]

            except ValueError:
                print(f'Not enough data of the combination {encoding}, {implementation_type} to split into trainings and test data')
                break

            # if there is training data for this combination, train a model
            if not model_train_data.empty:
                for type in model_types:
                    model_train_data, model_test_data = add_dummy_types(model_train_data.copy(), model_test_data.copy(), ['COMPRESSION_TYPE', 'SCAN_TYPE', 'DATA_TYPE'])
                    model = train_model(model_train_data, type)

                    model_name = f'{type}_{encoding}_{implementation_type}_model'
                    filename = f'{out}/models/split_{model_name}.sav'
                    joblib.dump(model, filename)

                    if not model_test_data.empty:
                        scores[model_name] = generate_model_plot(model, model_test_data, type, encoding, implementation_type, out)
    log(scores, out)


if __name__ == '__main__':
    args = parse_arguments()
    main(args)
