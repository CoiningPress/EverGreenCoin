#include "charitydialog.h"
#include "ui_charitydialog.h"
#include "walletmodel.h"
#include "addressbookpage.h"
#include "init.h"
#include "base58.h"
#include <QLineEdit>

StakeForCharityDialog::StakeForCharityDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakeForCharityDialog),
    model(0)
{
    ui->setupUi(this);

#if (QT_VERSION >= 0x040700)
    /* Do not move this to the XML file, Qt before 4.7 will choke on it */
    ui->charityPercentEdit->setPlaceholderText(tr(" % "));
    ui->charityAddressEdit->setPlaceholderText(tr("Enter Charity EverGreenCoin Account Address"));
    ui->charityMinEdit->setPlaceholderText(tr("Enter Minimum Amount (optional)"));
    ui->charityMaxEdit->setPlaceholderText(tr("Enter Maximum Amount (optional)"));
    ui->charityChangeAddressEdit->setPlaceholderText(tr("Enter Change Return EverGreenCoin Account Address (optional)"));
#endif

    ui->label_2->setFocus();
}

StakeForCharityDialog::~StakeForCharityDialog()
{
    delete ui;
}

void StakeForCharityDialog::setModel(WalletModel *model)
{
    this->model = model;

    CBitcoinAddress strAddress;
    CBitcoinAddress strChangeAddress;
    int nPer;
    qint64 nMin;
    qint64 nMax;

    model->getStakeForCharity(nPer, strAddress, strChangeAddress, nMin, nMax);

    if (strAddress.IsValid() && nPer > 0 )
    {
        ui->charityAddressEdit->setText(strAddress.ToString().c_str());
        ui->charityPercentEdit->setText(QString::number(nPer));
        if (strChangeAddress.IsValid())
            ui->charityChangeAddressEdit->setText(strChangeAddress.ToString().c_str());
        if (nMin > 0  && nMin != MIN_TX_FEE)
            ui->charityMinEdit->setText(QString::number(nMin/COIN));
        if (nMax > 0 && nMax != MAX_MONEY)
            ui->charityMaxEdit->setText(QString::number(nMax/COIN));
        ui->message->setStyleSheet("QLabel { color: green; font-weight: 900; }");
        ui->message->setText(tr("Thank you for giving to\n") + strAddress.ToString().c_str()+ " \n\n");
    }
}

void StakeForCharityDialog::setAddress(const QString &address)
{
    setAddress(address, ui->charityAddressEdit);
}

void StakeForCharityDialog::setAddress(const QString &address, QLineEdit *addrEdit)
{
    addrEdit->setText(address);
    addrEdit->setFocus();
}

void StakeForCharityDialog::on_addressBookButton_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
            setAddress(dlg.getReturnValue(), ui->charityAddressEdit);
    }
}

void StakeForCharityDialog::on_changeAddressBookButton_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
            setAddress(dlg.getReturnValue(), ui->charityChangeAddressEdit);
    }
}

void StakeForCharityDialog::on_enableButton_clicked()
{
    if(model->getEncryptionStatus() == WalletModel::Locked)
    {
        ui->message->setStyleSheet("QLabel { color: black; font-weight: 900;}");
        ui->message->setText(tr("Please unlock your software before starting \nEverGreenCoin Stake For Charity.")+ " \n\n");
        return;
    }

    bool fValidConversion = false;
    qint64 nMinAmount = MIN_TX_FEE;
    qint64 nMaxAmount = MAX_MONEY;
    CBitcoinAddress changeAddress = "";

    CBitcoinAddress address = ui->charityAddressEdit->text().toStdString();
    if (!address.IsValid())
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
        ui->message->setText(tr("The entered address:\n ") + ui->charityAddressEdit->text() + tr(" \nis invalid.\nPlease check the address and try again."));
        ui->charityAddressEdit->setFocus();
        return;
    }

    if (model->isMine(address))
    {
       ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
       ui->message->setText(tr("The entered address:\n ") + ui->charityAddressEdit->text() + tr(" \nis your own.\nPlease check the address and try again."));
       ui->charityAddressEdit->setFocus();
       return;
    }

    int nCharityPercent = ui->charityPercentEdit->text().toInt(&fValidConversion, 10);
    if (!fValidConversion || nCharityPercent > 100 || nCharityPercent <= 0)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
        ui->message->setText(tr("Please Enter 1 - 100 for percent.")+ " \n\n\n");
        ui->charityPercentEdit->setFocus();
        return;
    }

    if (!ui->charityMinEdit->text().isEmpty())
    {
        nMinAmount = ui->charityMinEdit->text().toDouble(&fValidConversion) * COIN;
        if(!fValidConversion || nMinAmount <= MIN_TX_FEE || nMinAmount >= MAX_MONEY  )
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
            ui->message->setText(tr("Min Amount is too low, please re-enter.")+ " \n\n\n");
            ui->charityMinEdit->setFocus();
            return;
        }
    }

    if (!ui->charityMaxEdit->text().isEmpty())
    {
        nMaxAmount = ui->charityMaxEdit->text().toDouble(&fValidConversion) * COIN;
        if(!fValidConversion || nMaxAmount <= MIN_TX_FEE || nMaxAmount >= MAX_MONEY  )
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
            ui->message->setText(tr("Max Amount is too high, please re-enter.")+ " \n\n\n");
            ui->charityMaxEdit->setFocus();
            return;
        }
    }

    if (nMinAmount >= nMaxAmount)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
        ui->message->setText(tr("Min Amount more than Max Amount, please re-enter.")+ " \n\n\n");
        ui->charityMinEdit->setFocus();
        return;
    }

    if (!ui->charityChangeAddressEdit->text().isEmpty())
    {
        changeAddress = ui->charityChangeAddressEdit->text().toStdString();
        if (!changeAddress.IsValid())
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
            ui->message->setText(tr("The entered change address:\n ") + ui->charityChangeAddressEdit->text() + tr(" \nis invalid.\nPlease check the address and try again."));
            ui->charityChangeAddressEdit->setFocus();
            return;
        }
        else if (!model->isMine(changeAddress))
        {
           ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
           ui->message->setText(tr("The entered change address:\n ") + ui->charityChangeAddressEdit->text() + tr(" \nis not owned.\nPlease check the address and try again."));
           ui->charityChangeAddressEdit->setFocus();
           return;
        }
    }

    model->setStakeForCharity(true, nCharityPercent, address, changeAddress, nMinAmount, nMaxAmount);
    if(!fGlobalStakeForCharity)
         fGlobalStakeForCharity = true;
    ui->message->setStyleSheet("QLabel { color: green; font-weight: 900;}");
    ui->message->setText(tr("Thank you for giving to:\n ") + QString(address.ToString().c_str()) + " \n\n");
    return;
}

void StakeForCharityDialog::on_disableButton_clicked()
{
    int nCharityPercent = 0;
    CBitcoinAddress address = "";
    CBitcoinAddress changeAddress = "";
    qint64 nMinAmount = MIN_TX_FEE;
    qint64 nMaxAmount = MAX_MONEY;
    fGlobalStakeForCharity = false;

    model->setStakeForCharity(false, nCharityPercent, address, changeAddress, nMinAmount, nMaxAmount);
    ui->charityAddressEdit->clear();
    ui->charityChangeAddressEdit->clear();
    ui->charityMaxEdit->clear();
    ui->charityMinEdit->clear();
    ui->charityPercentEdit->clear();
    ui->message->setStyleSheet("QLabel { color: black; font-weight: 900;}");
    ui->message->setText(tr("EverGreenCoin Stake For Charity is now off")+ " \n\n\n");
    return;
}
