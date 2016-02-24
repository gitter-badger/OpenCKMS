// ***********************************************************************
// Assembly         : 
// Author           : Edward Curren
// Created          : 02-22-2016
//
// Last Modified By : NCVEVC
// Last Modified On : 02-23-2016
// ***********************************************************************
// <copyright file="Cryptography.cpp" company="">
//     Copyright (c) . All rights reserved.
// </copyright>
// <summary></summary>
// ***********************************************************************

#include "stdafx.h"
#include "Cryptography.h"
#include <cryptlib.h>


using namespace System::Runtime::InteropServices;
using namespace OpenCKMS;

/// <summary>
/// Evaluates the method result.
/// </summary>
/// <param name="result">The result.</param>
void EvaluateMethodResult(int result)
{
	if (result)
	{
		int errorStringLength;
		cryptGetAttributeString(result, CRYPT_ATTRIBUTE_ERRORMESSAGE, NULL, &errorStringLength);
		char *errorString = new char[errorStringLength];
		cryptGetAttributeString(result, CRYPT_ATTRIBUTE_ERRORMESSAGE, errorString, &errorStringLength);
		String^ errorDescription = gcnew String(errorString);
		throw gcnew CryptographicException(errorDescription);
		delete errorString;
	}
}



/// <summary>
/// Initializes a new instance of the <see cref="Cryptography" /> class.
/// </summary>
/// <param name="">The .</param>
Cryptography::Cryptography(const Cryptography%) {
	Resources::ResourceManager^ rm = gcnew Resources::ResourceManager(L"OpenCKMS.CryptographyExceptionMessages", this->GetType()->Assembly);
	throw gcnew System::InvalidOperationException(rm->GetString(L"NoCopyConstructor"));
}

/// <summary>
/// Initializes a new instance of the <see cref="Cryptography" /> class.
/// </summary>
OpenCKMS::Cryptography::Cryptography()
{
	EvaluateMethodResult(cryptInit());
}

/// <summary>
/// Finalizes an instance of the <see cref="Cryptography" /> class.
/// </summary>
OpenCKMS::Cryptography::~Cryptography()
{
	EvaluateMethodResult(cryptEnd());
}

/// <summary>
/// Queries the capability.
/// </summary>
/// <param name="algorithm">The algorithm.</param>
/// <returns>OpenCKMS.AlgorithmCapabilities ^.</returns>
AlgorithmCapabilities ^ OpenCKMS::Cryptography::QueryCapability(Algorithm algorithm)
{
	/*auto algorithmCapabilities = gcnew AlgorithmCapabilities();
	return algorithmCapabilities;*/

	throw gcnew NotImplementedException("QueryCapability is not yet implemented.");
}

/// <summary>
/// Creates the context.
/// </summary>
/// <param name="user">The user.</param>
/// <param name="algorithm">The algorithm.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::CreateContext(CryptUser user, Algorithm algorithm)
{
	CryptContext context;
	int result = cryptCreateContext(&context, Unused, (CRYPT_ALGO_TYPE)algorithm);
	if(result) {
		switch(result) {
			case -2:
				throw gcnew Exception("Error with user passed into method.");
			break;
			case -3:
				throw gcnew Exception("Error with algorithm passed into method.");
			break;
			default:
			String^ errorMessage = gcnew String("An error occurred in the CreateContext method.  The returned error code is " + result);
			throw gcnew CryptographicException(errorMessage);
		}
	}
	return context;
}

/// <summary>
/// Destroys the context.
/// </summary>
/// <param name="context">The context.</param>
void OpenCKMS::Cryptography::DestroyContext(CryptContext context)
{
	EvaluateMethodResult(cryptDestroyContext(context));
}

/// <summary>
/// Destroys the object.
/// </summary>
/// <param name="object">The object.</param>
void Cryptography::DestroyObject(CryptObject object)
{
	EvaluateMethodResult(cryptDestroyObject(object));
}

/// <summary>
/// Generates the key.
/// </summary>
/// <param name="context">The context.</param>
/// <param name="label">The label.</param>
void OpenCKMS::Cryptography::GenerateKey(CryptContext context, String^ label)
{
	SetAttribute(context, AttributeType::CtxInfoLabel, label);
	int result = cryptGenerateKey(context);
}

/// <summary>
/// Encrypts the specified context.
/// </summary>
/// <param name="context">The context.</param>
/// <param name="data">The data.</param>
/// <returns>array&lt;System.Byte&gt;^.</returns>
array<System::Byte>^ OpenCKMS::Cryptography::Encrypt(CryptContext context, String^ data)
{
	IntPtr ptrToNativeString = Marshal::StringToHGlobalAnsi(data);
	try
	{		
		int result = cryptEncrypt(context, (void*)static_cast<char*>(ptrToNativeString.ToPointer()), data->Length);
		auto encryptedData = gcnew array<Byte>(10);
		return encryptedData;
	}
	catch(...)
	{
		throw gcnew Exception("Encryption failure.");
	}
	finally {
		Marshal::FreeHGlobal(ptrToNativeString);
	}	
}

/// <summary>
/// Encrypts the specified context.
/// </summary>
/// <param name="context">The context.</param>
/// <param name="data">The data.</param>
/// <returns>array&lt;System.Byte&gt;^.</returns>
array<System::Byte>^ OpenCKMS::Cryptography::Encrypt(CryptContext context, array<Byte>^ data)
{
	auto encryptedData = gcnew array<Byte>(10);
	return encryptedData;
}

/// <summary>
/// Decrypts the specified context.
/// </summary>
/// <param name="context">The context.</param>
/// <param name="data">The data.</param>
/// <param name="dataLength">Length of the data.</param>
/// <returns>array&lt;System.Byte&gt;^.</returns>
array<System::Byte>^ OpenCKMS::Cryptography::Decrypt(CryptContext context, array<Byte>^ data, int dataLength)
{
	auto encryptedData = gcnew array<Byte>(10);
	return encryptedData;
}

/// <summary>
/// Sets the attribute.
/// </summary>
/// <param name="handle">The handle.</param>
/// <param name="attributeType">Type of the attribute.</param>
/// <param name="value">The value.</param>
void OpenCKMS::Cryptography::SetAttribute(CryptHandle handle, AttributeType attributeType, int value)
{

}

/// <summary>
/// Sets the attribute.
/// </summary>
/// <param name="handle">The handle.</param>
/// <param name="attributeType">Type of the attribute.</param>
/// <param name="value">The value.</param>
void OpenCKMS::Cryptography::SetAttribute(CryptHandle handle, AttributeType attributeType, String^ value)
{
	IntPtr ptrToNativeString = Marshal::StringToHGlobalAnsi(value);
	cryptSetAttributeString(handle, static_cast<CRYPT_ATTRIBUTE_TYPE>(attributeType), static_cast<char*>(ptrToNativeString.ToPointer()), value->Length);
	Marshal::FreeHGlobal(ptrToNativeString);
}

/// <summary>
/// Gets the attribute.
/// </summary>
/// <param name="handle">The handle.</param>
/// <param name="attributeType">Type of the attribute.</param>
/// <returns>int.</returns>
int Cryptography::GetAttribute(CryptHandle handle, AttributeType attributeType)
{
	return 0;
}

/// <summary>
/// Gets the attribute string.
/// </summary>
/// <param name="handle">The handle.</param>
/// <param name="attributeType">Type of the attribute.</param>
/// <returns>System.String ^.</returns>
String^ OpenCKMS::Cryptography::GetAttributeString(CryptHandle handle, AttributeType attributeType)
{
	return String::Empty;
}

/// <summary>
/// Deletes the attribute.
/// </summary>
/// <param name="handle">The handle.</param>
/// <param name="attributeType">Type of the attribute.</param>
void OpenCKMS::Cryptography::DeleteAttribute(CryptHandle handle, AttributeType attributeType)
{

}

/****************************************************************************
*																			*
*						Mid-level Encryption Functions						*
*																			*
****************************************************************************/

/* Export and import an encrypted session key */

/// <summary>
/// Exports the key.
/// </summary>
/// <param name="exportKey">The export key.</param>
/// <param name="sessionKeyContext">The session key context.</param>
/// <returns>array&lt;Byte&gt;^.</returns>
array<Byte>^ Cryptography::ExportKey(CryptHandle exportKey, CryptContext sessionKeyContext)
{
	int encryptedKeyLength;
	CryptContext context;

	int result = cryptExportKey(NULL, 0, &encryptedKeyLength, exportKey, sessionKeyContext);
	if(result != 0) {
		// Error goes here
		throw gcnew System::Exception("Bad stuff.");
	}
	char* buffer = new char[encryptedKeyLength];
	result = cryptExportKey(buffer, 10240, &encryptedKeyLength, exportKey, sessionKeyContext);
	array<Byte>^ key = gcnew array<Byte>(encryptedKeyLength);
	Marshal::Copy((IntPtr)buffer, key, 0, encryptedKeyLength);
	delete buffer;
	return key;
}

/// <summary>
/// Exports the key.
/// </summary>
/// <param name="exportKey">The export key.</param>
/// <param name="maximumKeyLength">Maximum length of the key.</param>
/// <param name="keyLength">Length of the key.</param>
/// <param name="keyFormat">The key format.</param>
/// <param name="exportKeyHandle">The export key handle.</param>
/// <param name="sessionKeyContext">The session key context.</param>
/// <returns>array&lt;Byte&gt;^.</returns>
array<Byte>^ OpenCKMS::Cryptography::ExportKey(CryptHandle exportKey, int maximumKeyLength, int keyLength,
	Format keyFormat, CryptHandle exportKeyHandle,
	CryptContext sessionKeyContext)
{
	return gcnew array<Byte>(0);
}

/// <summary>
/// Imports the key.
/// </summary>
/// <param name="encryptedKey">The encrypted key.</param>
/// <param name="encryptedKeyLength">Length of the encrypted key.</param>
/// <param name="importKeyContext">The import key context.</param>
/// <param name="sessionKeyContext">The session key context.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::ImportKey(array<Byte>^ encryptedKey, int encryptedKeyLength, CryptContext importKeyContext,
	SessionContext sessionKeyContext)
{
	return 0;
}

/* Create and check a digital signature */

/// <summary>
/// Creates the signature.
/// </summary>
/// <param name="signatureMaxLength">Maximum length of the signature.</param>
/// <param name="formatType">Type of the format.</param>
/// <param name="signatureContext">The signature context.</param>
/// <param name="hashContext">The hash context.</param>
/// <param name="extraData">The extra data.</param>
/// <returns>array&lt;Byte&gt;^.</returns>
array<Byte>^ OpenCKMS::Cryptography::CreateSignature(int signatureMaxLength, Format formatType, CryptContext signatureContext,
	CryptContext hashContext, CryptCertificate extraData)
{
	return gcnew array<Byte>(0);
}

/// <summary>
/// Checks the signature.
/// </summary>
/// <param name="signature">The signature.</param>
/// <param name="signatureLength">Length of the signature.</param>
/// <param name="signatureCheckKey">The signature check key.</param>
/// <param name="hashContext">The hash context.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::CheckSignature(array<Byte>^ signature, int signatureLength, CryptHandle signatureCheckKey,
	CryptContext hashContext)
{
	return 0;
}

/// <summary>
/// Keysets the open.
/// </summary>
/// <param name="keysetType">Type of the keyset.</param>
/// <param name="name">The name.</param>
/// <param name="keysetOptions">The keyset options.</param>
/// <returns>CryptKeyset.</returns>
CryptKeyset OpenCKMS::Cryptography::KeysetOpen(KeysetType^ keysetType, String ^ name, KeysetOption keysetOptions)
{
	return CryptKeyset();
}

/// <summary>
/// Keysets the close.
/// </summary>
/// <param name="keyset">The keyset.</param>
void OpenCKMS::Cryptography::KeysetClose(CryptKeyset keyset)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Gets the public key.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="keyIdType">Type of the key identifier.</param>
/// <param name="KeyId">The key identifier.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::GetPublicKey(CryptKeyset keyset, KeyIdType keyIdType, String ^ KeyId)
{
	return CryptContext();
}

/// <summary>
/// Gets the private key.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="keyIdType">Type of the key identifier.</param>
/// <param name="keyId">The key identifier.</param>
/// <param name="password">The password.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::GetPrivateKey(CryptKeyset keyset, KeyIdType keyIdType, String ^ keyId, String ^ password)
{
	return CryptContext();
}

/// <summary>
/// Adds the public key.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="certificate">The certificate.</param>
void OpenCKMS::Cryptography::AddPublicKey(CryptKeyset keyset, CryptCertificate certificate)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Adds the private key.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="key">The key.</param>
/// <param name="password">The password.</param>
void OpenCKMS::Cryptography::AddPrivateKey(CryptKeyset keyset, CryptHandle key, String ^ password)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Deletes the key.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="keyIdType">Type of the key identifier.</param>
/// <param name="keyId">The key identifier.</param>
void OpenCKMS::Cryptography::DeleteKey(CryptKeyset keyset, KeyIdType keyIdType, String ^ keyId)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Creates the certificate.
/// </summary>
/// <param name="user">The user.</param>
/// <param name="certificateType">Type of the certificate.</param>
/// <returns>CryptCertificate.</returns>
CryptCertificate OpenCKMS::Cryptography::CreateCertificate(CryptUser user, CertificateType certificateType)
{
	return CryptCertificate();
}

/// <summary>
/// Destroys the certificate.
/// </summary>
/// <param name="certificate">The certificate.</param>
void OpenCKMS::Cryptography::DestroyCertificate(CryptCertificate certificate)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Gets the certificate extension.
/// </summary>
/// <param name="certificate">The certificate.</param>
/// <param name="oid">The oid.</param>
/// <param name="extensionMaximumLength">Maximum length of the extension.</param>
/// <returns>OpenCKMS.CertificateExtension.</returns>
CertificateExtension OpenCKMS::Cryptography::GetCertificateExtension(CryptCertificate certificate, String ^ oid, int extensionMaximumLength)
{
	return CertificateExtension();
}

/// <summary>
/// Adds the certificate extension.
/// </summary>
/// <param name="certificate">The certificate.</param>
/// <param name="oid">The oid.</param>
/// <param name="isCritical">The is critical.</param>
/// <param name="extension">The extension.</param>
/// <param name="extensionMaximumLength">Maximum length of the extension.</param>
void OpenCKMS::Cryptography::AddCertificateExtension(CryptCertificate certificate, String ^ oid, bool isCritical, String ^ extension, int extensionMaximumLength)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Deletes the certificate extension.
/// </summary>
/// <param name="certificate">The certificate.</param>
/// <param name="oid">The oid.</param>
void OpenCKMS::Cryptography::DeleteCertificateExtension(CryptCertificate certificate, String ^ oid)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Signs the certificate.
/// </summary>
/// <param name="certificate">The certificate.</param>
/// <param name="certificateContext">The certificate context.</param>
void OpenCKMS::Cryptography::SignCertificate(CryptCertificate certificate, CryptContext certificateContext)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Checks the certificate signature.
/// </summary>
/// <param name="certificate">The certificate.</param>
/// <param name="signatureCheckKey">The signature check key.</param>
void OpenCKMS::Cryptography::CheckCertificateSignature(CryptCertificate certificate, CryptHandle signatureCheckKey)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Imports the certificate.
/// </summary>
/// <param name="certificateObject">The certificate object.</param>
/// <param name="certificateObjectLength">Length of the certificate object.</param>
/// <param name="user">The user.</param>
/// <returns>CryptCertificate.</returns>
CryptCertificate OpenCKMS::Cryptography::ImportCertificate(array<Byte>^ certificateObject, int certificateObjectLength, CryptUser user)
{
	return CryptCertificate();
}

/// <summary>
/// Exports the certificate.
/// </summary>
/// <param name="certificateObjectMaxLength">Maximum length of the certificate object.</param>
/// <param name="certificateType">Type of the certificate.</param>
/// <param name="certificate">The certificate.</param>
/// <returns>array&lt;Byte&gt;^.</returns>
array<Byte>^ OpenCKMS::Cryptography::ExportCertificate(int certificateObjectMaxLength, CertificateType certificateType, CryptCertificate certificate)
{
	throw gcnew System::NotImplementedException();
	// TODO: insert return statement here
}

/// <summary>
/// Adds the certification authority item.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="certificate">The certificate.</param>
void OpenCKMS::Cryptography::AddCertificationAuthorityItem(CryptKeyset keyset, CryptCertificate certificate)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Gets the certification authority item.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="certificateType">Type of the certificate.</param>
/// <param name="keyIdType">Type of the key identifier.</param>
/// <param name="keyId">The key identifier.</param>
/// <returns>CryptCertificate.</returns>
CryptCertificate OpenCKMS::Cryptography::GetCertificationAuthorityItem(CryptKeyset keyset, CertificateType certificateType, KeyIdType keyIdType, String ^ keyId)
{
	return CryptCertificate();
}

/// <summary>
/// Deletes the certification authority item.
/// </summary>
/// <param name="keyset">The keyset.</param>
/// <param name="certificateType">Type of the certificate.</param>
/// <param name="keyIdType">Type of the key identifier.</param>
/// <param name="keyId">The key identifier.</param>
void OpenCKMS::Cryptography::DeleteCertificationAuthorityItem(CryptKeyset keyset, CertificateType certificateType, KeyIdType keyIdType, String ^ keyId)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Certifications the authority management.
/// </summary>
/// <param name="action">The action.</param>
/// <param name="keyset">The keyset.</param>
/// <param name="caKey">The ca key.</param>
/// <param name="certificateRequest">The certificate request.</param>
/// <returns>CryptCertificate.</returns>
CryptCertificate OpenCKMS::Cryptography::CertificationAuthorityManagement(CertificateActionType action, CryptKeyset keyset, CryptContext caKey, CryptCertificate certificateRequest)
{
	return CryptCertificate();
}

/// <summary>
/// Creates the envelope.
/// </summary>
/// <param name="user">The user.</param>
/// <param name="format">The format.</param>
/// <returns>CryptEnvelope.</returns>
CryptEnvelope OpenCKMS::Cryptography::CreateEnvelope(CryptUser user, Format format)
{
	return CryptEnvelope();
}

/// <summary>
/// Destroys the envelope.
/// </summary>
/// <param name="envelope">The envelope.</param>
void OpenCKMS::Cryptography::DestroyEnvelope(CryptEnvelope envelope)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Pushes the data.
/// </summary>
/// <param name="envelope">The envelope.</param>
/// <param name="data">The data.</param>
void OpenCKMS::Cryptography::PushData(CryptHandle envelope, array<Byte>^ data)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Pops the data.
/// </summary>
/// <param name="envelope">The envelope.</param>
/// <param name="length">The length.</param>
/// <returns>array&lt;Byte&gt;^.</returns>
array<Byte>^ OpenCKMS::Cryptography::PopData(CryptEnvelope envelope, int length)
{
	throw gcnew System::NotImplementedException();
	// TODO: insert return statement here
}

/// <summary>
/// Opens the device.
/// </summary>
/// <param name="user">The user.</param>
/// <param name="device">The device.</param>
/// <param name="name">The name.</param>
/// <returns>CryptDevice.</returns>
CryptDevice OpenCKMS::Cryptography::OpenDevice(CryptUser user, CryptDevice device, String ^ name)
{
	return CryptDevice();
}

/// <summary>
/// Closes the device.
/// </summary>
/// <param name="device">The device.</param>
void OpenCKMS::Cryptography::CloseDevice(CryptDevice device)
{
	throw gcnew System::NotImplementedException();
}

/// <summary>
/// Queries the device capabilities.
/// </summary>
/// <param name="device">The device.</param>
/// <param name="algorithm">The algorithm.</param>
/// <returns>OpenCKMS.QueryInfo.</returns>
QueryInfo OpenCKMS::Cryptography::QueryDeviceCapabilities(CryptDevice device, Algorithm algorithm)
{
	return QueryInfo();
}

/// <summary>
/// Creates the device context.
/// </summary>
/// <param name="device">The device.</param>
/// <param name="algorithm">The algorithm.</param>
/// <returns>CryptContext.</returns>
CryptContext OpenCKMS::Cryptography::CreateDeviceContext(CryptDevice device, Algorithm algorithm)
{
	return CryptContext();
}

/// <summary>
/// Logins the specified user.
/// </summary>
/// <param name="user">The user.</param>
/// <param name="password">The password.</param>
/// <returns>CryptUser.</returns>
CryptUser OpenCKMS::Cryptography::Login(String ^ user, String ^ password)
{
	return CryptUser();
}

/// <summary>
/// Logouts the specified user.
/// </summary>
/// <param name="user">The user.</param>
void OpenCKMS::Cryptography::Logout(CryptUser user)
{
	throw gcnew System::NotImplementedException();
}


