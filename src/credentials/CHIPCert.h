/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file defines data types and objects for modeling and
 *      working with CHIP certificates.
 *
 */

#pragma once

#include <string.h>

#include <asn1/ASN1.h>
#include <core/CHIPConfig.h>
#include <core/CHIPTLV.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/PeerId.h>
#include <support/BitFlags.h>
#include <support/DLLUtil.h>

namespace chip {
namespace Credentials {

static constexpr uint32_t kKeyIdentifierLength                 = 20;
static constexpr uint32_t kChip32bitAttrUTF8Length             = 8;
static constexpr uint32_t kChip64bitAttrUTF8Length             = 16;
static constexpr uint16_t kX509NoWellDefinedExpirationDateYear = 9999;

// As per specifications (6.3.5. Node Operational Credentials Certificates)
static constexpr uint32_t kMaxCHIPCertLength = 400;
static constexpr uint32_t kMaxDERCertLength  = 600;

// The decode buffer is used to reconstruct TBS section of X.509 certificate, which doesn't include signature.
static constexpr uint32_t kMaxCHIPCertDecodeBufLength = kMaxDERCertLength - Crypto::kMax_ECDSA_Signature_Length_Der;

/** Data Element Tags for the CHIP Certificate
 */
enum
{
    // ---- Context-specific Tags for ChipCertificate Structure ----
    kTag_SerialNumber            = 1,  /**< [ byte string ] Certificate serial number, in BER integer encoding. */
    kTag_SignatureAlgorithm      = 2,  /**< [ unsigned int ] Enumerated value identifying the certificate signature algorithm. */
    kTag_Issuer                  = 3,  /**< [ list ] The issuer distinguished name of the certificate. */
    kTag_NotBefore               = 4,  /**< [ unsigned int ] Certificate validity period start (certificate date format). */
    kTag_NotAfter                = 5,  /**< [ unsigned int ] Certificate validity period end (certificate date format). */
    kTag_Subject                 = 6,  /**< [ list ] The subject distinguished name of the certificate. */
    kTag_PublicKeyAlgorithm      = 7,  /**< [ unsigned int ] Identifies the algorithm with which the public key can be used. */
    kTag_EllipticCurveIdentifier = 8,  /**< [ unsigned int ] For EC certs, identifies the elliptic curve used. */
    kTag_EllipticCurvePublicKey  = 9,  /**< [ byte string ] The elliptic curve public key, in X9.62 encoded format. */
    kTag_Extensions              = 10, /**< [ list ] Certificate extensions. */
    kTag_ECDSASignature          = 11, /**< [ byte string ] The ECDSA signature for the certificate. */

    // ---- Context-specific Tags for certificate extensions ----
    kTag_BasicConstraints       = 1, /**< [ structure ] Identifies whether the subject of the certificate is a CA. */
    kTag_KeyUsage               = 2, /**< [ unsigned int ] Bits identifying key usage, per RFC5280. */
    kTag_ExtendedKeyUsage       = 3, /**< [ array ] Enumerated values giving the purposes for which the public key can be used. */
    kTag_SubjectKeyIdentifier   = 4, /**< [ byte string ] Identifier of the certificate's public key. */
    kTag_AuthorityKeyIdentifier = 5, /**< [ byte string ] Identifier of the public key used to sign the certificate. */
    kTag_FutureExtension        = 6, /**< [ byte string ] Arbitrary extension. DER encoded SEQUENCE as in X.509 form. */

    // ---- Context-specific Tags for BasicConstraints Structure ----
    kTag_BasicConstraints_IsCA = 1,              /**< [ boolean ] True if the certificate can be used to verify certificate
                                                    signatures. */
    kTag_BasicConstraints_PathLenConstraint = 2, /**< [ unsigned int ] Maximum number of subordinate intermediate certificates. */
};

/** Identifies the purpose or application of certificate
 *
 * A certificate type is a label that describes a certificate's purpose or application.
 * Certificate types are not carried as attributes of the corresponding certificates, but
 * rather are derived from the certificate's structure and/or the context in which it is used.
 * The certificate type enumeration includes a set of pre-defined values describing commonly
 * used certificate applications.
 *
 * Certificate types are primarily used in the implementation of access control policies,
 * where access to application features is influenced by the type of certificate presented
 * by a requester.
 *
 * @note Cert type is an API data type only; it should never be sent over-the-wire.
 */
enum
{
    kCertType_NotSpecified    = 0x00, /**< The certificate's type has not been specified. */
    kCertType_Root            = 0x01, /**< A CHIP Root certificate. */
    kCertType_ICA             = 0x02, /**< A CHIP Intermediate CA certificate. */
    kCertType_Node            = 0x03, /**< A CHIP node certificate. */
    kCertType_FirmwareSigning = 0x04, /**< A CHIP firmware signing certificate. */
};

/** X.509 Certificate Key Purpose Flags
 *
 *    The flags order must match the enumeration order of corresponding kOID_KeyPurpose values.
 */
enum class KeyPurposeFlags : uint8_t
{
    kServerAuth      = 0x01, /**< Extended key usage is server authentication. */
    kClientAuth      = 0x02, /**< Extended key usage is client authentication. */
    kCodeSigning     = 0x04, /**< Extended key usage is code signing. */
    kEmailProtection = 0x08, /**< Extended key usage is email protection. */
    kTimeStamping    = 0x10, /**< Extended key usage is time stamping. */
    kOCSPSigning     = 0x20, /**< Extended key usage is OCSP signing. */
};

/** X.509 Certificate Key Usage Flags
 */
enum class KeyUsageFlags : uint16_t
{
    kDigitalSignature = 0x0001, /**< Key usage is digital signature. */
    kNonRepudiation   = 0x0002, /**< Key usage is non-repudiation. */
    kKeyEncipherment  = 0x0004, /**< Key usage is key encipherment. */
    kDataEncipherment = 0x0008, /**< Key usage is data encipherment. */
    kKeyAgreement     = 0x0010, /**< Key usage is key agreement. */
    kKeyCertSign      = 0x0020, /**< Key usage is key cert signing. */
    kCRLSign          = 0x0040, /**< Key usage is CRL signing. */
    kEncipherOnly     = 0x0080, /**< Key usage is encipher only. */
    kDecipherOnly     = 0x0100, /**< Key usage is decipher only. */
};

/** CHIP Certificate Flags
 *
 * Contains information about a certificate that has been loaded into a ChipCertificateData object.
 */
enum class CertFlags : uint16_t
{
    kExtPresent_BasicConstraints = 0x0001, /**< Basic constraints extension is present in the certificate. */
    kExtPresent_KeyUsage         = 0x0002, /**< Key usage extension is present in the certificate. */
    kExtPresent_ExtendedKeyUsage = 0x0004, /**< Extended key usage extension is present in the certificate. */
    kExtPresent_SubjectKeyId     = 0x0008, /**< Subject key identifier extension is present in the certificate. */
    kExtPresent_AuthKeyId        = 0x0010, /**< Authority key identifier extension is present in the certificate. */
    kExtPresent_FutureIsCritical = 0x0020, /**< Future extension marked as critical is present in the certificate. */
    kPathLenConstraintPresent    = 0x0040, /**< Path length constraint is present in the certificate. */
    kIsCA                        = 0x0080, /**< Indicates that certificate is a CA certificate. */
    kIsTrustAnchor               = 0x0100, /**< Indicates that certificate is a trust anchor. */
    kTBSHashPresent              = 0x0200, /**< Indicates that TBS hash of the certificate was generated and stored. */
};

/** CHIP Certificate Decode Flags
 *
 * Contains information specifying how a certificate should be decoded.
 */
enum class CertDecodeFlags : uint8_t
{
    kGenerateTBSHash = 0x01, /**< Indicates that to-be-signed (TBS) hash of the certificate should be calculated when certificate is
                                loaded. The TBS hash is then used to validate certificate signature. Normally, all certificates
                                (except trust anchor) in the certificate validation chain require TBS hash. */
    kIsTrustAnchor = 0x02,   /**< Indicates that the corresponding certificate is trust anchor. */
};

/** CHIP Certificate Validate Flags
 *
 * Contains information specifying how a certificate should be validated.
 */
enum class CertValidateFlags : uint8_t
{
    kIgnoreNotBefore = 0x01, /**< Indicate that a Not Before field should be ignored when doing certificate validation. This
                                flag applies to all certificates in the validation chain. */
    kIgnoreNotAfter = 0x02,  /**< Indicate that a Not After field should be ignored when doing certificate validation. This
                                flag applies to all certificates in the validation chain. */
};

enum
{
    kNullCertTime = 0
};

/**
 *  @struct ChipRDN
 *
 *  @brief
 *    A data structure representing a Relative Distinguished Name (RDN) in a CHIP certificate.
 */
struct ChipRDN
{
    ByteSpan mString;         /**< Attribute value when encoded as a string. */
    uint64_t mChipVal;        /**< CHIP specific DN attribute value. */
    chip::ASN1::OID mAttrOID; /**< DN attribute CHIP OID. */

    bool IsEqual(const ChipRDN & other) const;
    bool IsEmpty() const { return mAttrOID == chip::ASN1::kOID_NotSpecified; }
    void Clear() { mAttrOID = chip::ASN1::kOID_NotSpecified; }
};

/**
 *  @brief
 *    A data structure representing a Distinguished Name (DN) in a CHIP certificate.
 */
class ChipDN
{
public:
    ChipDN();
    ~ChipDN();

    void Clear();

    /**
     * @brief Add CHIP-specific attribute to the DN.
     *
     * @param oid     CHIP-specific OID for DN attribute.
     * @param val     CHIP-specific DN attribute value.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR AddAttribute(chip::ASN1::OID oid, uint64_t val);

    /**
     * @brief Add string attribute to the DN.
     *
     * @param oid     String OID for DN attribute.
     * @param val     A ByteSpan object containing a pointer and length of the DN string attribute
     *                buffer. The value in the buffer should remain valid while the object is in use.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR AddAttribute(chip::ASN1::OID oid, ByteSpan val);

    /**
     * @brief Determine type of a CHIP certificate.
     *        This method performs an assessment of a certificate's type based on the structure
     *        of its subject DN.
     *
     * @param certType  A reference to the certificate type value.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR GetCertType(uint8_t & certType) const;

    /**
     * @brief Retrieve the ID of a CHIP certificate.
     *
     * @param certId  A reference to the certificate ID value.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR GetCertChipId(uint64_t & certId) const;

    bool IsEqual(const ChipDN & other) const;

    /**
     * @brief Determine if DN is empty (doesn't have DN attributes).
     *
     * @return true if DN is empty, false otherwise.
     **/
    bool IsEmpty() const { return RDNCount() == 0; }

    ChipRDN rdn[CHIP_CONFIG_CERT_MAX_RDN_ATTRIBUTES];

    uint8_t RDNCount() const;
};

/**
 *  @brief  A data structure for holding a certificate key identifier, without the ownership of it.
 */
using CertificateKeyId = FixedByteSpan<kKeyIdentifierLength>;

/**
 *  @brief  A data structure for holding a P256 ECDSA signature, without the ownership of it.
 */
using P256ECDSASignatureSpan = FixedByteSpan<Crypto::kP256_ECDSA_Signature_Length_Raw>;

/**
 *  @brief  A data structure for holding a P256 Public Key, without the ownership of it.
 */
using P256PublicKeySpan = FixedByteSpan<Crypto::kP256_Point_Length>;

/**
 *  @brief  A data structure for holding a P256 Integer, without the ownership of it.
 */
using P256IntegerSpan = FixedByteSpan<Crypto::kP256_FE_Length>;

/**
 *  @struct ChipCertificateData
 *
 *  @brief
 *    In-memory representation of data extracted from a CHIP certificate.
 *
 *    Some of the fields in this structure are pointers to the fields in the original
 *    CHIP certificate. That CHIP certificate is stored in a separate buffer and it is
 *    required that data in that buffer remains valid while the corresponding
 *    ChipCertificateData structure is used.
 */
struct ChipCertificateData
{
    ChipCertificateData();
    ~ChipCertificateData();

    void Clear();
    bool IsEqual(const ChipCertificateData & other) const;

    ByteSpan mCertificate;                      /**< Original raw buffer data. */
    ChipDN mSubjectDN;                          /**< Certificate Subject DN. */
    ChipDN mIssuerDN;                           /**< Certificate Issuer DN. */
    CertificateKeyId mSubjectKeyId;             /**< Certificate Subject public key identifier. */
    CertificateKeyId mAuthKeyId;                /**< Certificate Authority public key identifier. */
    uint32_t mNotBeforeTime;                    /**< Certificate validity: Not Before field. */
    uint32_t mNotAfterTime;                     /**< Certificate validity: Not After field. */
    P256PublicKeySpan mPublicKey;               /**< Certificate public key. */
    uint16_t mPubKeyCurveOID;                   /**< Public key Elliptic Curve CHIP OID. */
    uint16_t mPubKeyAlgoOID;                    /**< Public key algorithm CHIP OID. */
    uint16_t mSigAlgoOID;                       /**< Certificate signature algorithm CHIP OID. */
    BitFlags<CertFlags> mCertFlags;             /**< Certificate data flags. */
    BitFlags<KeyUsageFlags> mKeyUsageFlags;     /**< Certificate key usage extensions flags. */
    BitFlags<KeyPurposeFlags> mKeyPurposeFlags; /**< Certificate extended key usage extensions flags. */
    uint8_t mPathLenConstraint;                 /**< Basic constraint: path length. */
    P256ECDSASignatureSpan mSignature;          /**< Certificate signature. */

    uint8_t mTBSHash[Crypto::kSHA256_Hash_Length]; /**< Certificate TBS hash. */
};

/**
 *  @struct ValidationContext
 *
 *  @brief
 *    Context information used during certification validation.
 */
struct ValidationContext
{
    uint32_t mEffectiveTime;                        /**< Current CHIP Epoch UTC time. */
    const ChipCertificateData * mTrustAnchor;       /**< Pointer to the Trust Anchor Certificate data structure.
                                                       This value is set during certificate validation process
                                                       to indicate to the caller the trust anchor of the
                                                       validated certificate. */
    BitFlags<KeyUsageFlags> mRequiredKeyUsages;     /**< Key usage extensions that should be present in the
                                                       validated certificate. */
    BitFlags<KeyPurposeFlags> mRequiredKeyPurposes; /**< Extended Key usage extensions that should be present
                                                       in the validated certificate. */
    BitFlags<CertValidateFlags> mValidateFlags;     /**< Certificate validation flags, specifying how a certificate
                                                       should be validated. */
    uint8_t mRequiredCertType;                      /**< Required certificate type. */

    void Reset();
};

/**
 *  @class ChipCertificateSet
 *
 *  @brief
 *    Collection of CHIP certificate data providing methods for
 *    certificate validation and signature verification.
 */
class DLL_EXPORT ChipCertificateSet
{
public:
    ChipCertificateSet();
    ~ChipCertificateSet();

    ChipCertificateSet & operator=(ChipCertificateSet && aOther)
    {
        mCerts               = aOther.mCerts;
        aOther.mCerts        = nullptr;
        mCertCount           = aOther.mCertCount;
        mMaxCerts            = aOther.mMaxCerts;
        mMemoryAllocInternal = aOther.mMemoryAllocInternal;

        return *this;
    }

    /**
     * @brief Initialize ChipCertificateSet.
     *        This initialization method is used when all memory structures needed for operation are
     *        allocated internally using chip::Platform::MemoryAlloc() and freed with chip::Platform::MemoryFree().
     *
     * @param maxCertsArraySize  Maximum number of CHIP certificates to be loaded to the set.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR Init(uint8_t maxCertsArraySize);

    /**
     * @brief Initialize ChipCertificateSet.
     *        This initialization method is used when all memory structures needed for operation are
     *        allocated externally and methods in this class don't need to deal with memory allocations.
     *
     * @param certsArray      A pointer to the array of the ChipCertificateData structures.
     * @param certsArraySize  Number of ChipCertificateData entries in the array.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR Init(ChipCertificateData * certsArray, uint8_t certsArraySize);

    /**
     * @brief Release resources allocated by this class.
     **/
    void Release();

    /**
     * @brief Clear certificate data loaded into this set.
     **/
    void Clear();

    /**
     * @brief Load CHIP certificate into set.
     *        It is required that the CHIP certificate in the chipCert buffer stays valid while
     *        the certificate data in the set is used.
     *        In case of an error the certificate set is left in the same state as prior to this call.
     *
     * @param chipCert     Buffer containing certificate encoded in CHIP format.
     * @param chipCertLen  The length of the certificate buffer.
     * @param decodeFlags  Certificate decoding option flags.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR LoadCert(const uint8_t * chipCert, uint32_t chipCertLen, BitFlags<CertDecodeFlags> decodeFlags);

    /**
     * @brief Load CHIP certificate into set.
     *        It is required that the CHIP certificate in the reader's underlying buffer stays valid while
     *        the certificate data in the set is used.
     *        In case of an error the certificate set is left in the same state as prior to this call.
     *
     * @param reader       A TLVReader positioned at the CHIP certificate TLV structure.
     * @param decodeFlags  Certificate decoding option flags.
     * @param chipCert     Buffer containing certificate encoded on CHIP format. It is required that this CHIP certificate
     *                     in chipCert ByteSpan stays valid while the certificate data in the set is used.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR LoadCert(chip::TLV::TLVReader & reader, BitFlags<CertDecodeFlags> decodeFlags, ByteSpan chipCert = ByteSpan());

    /**
     * @brief Load CHIP certificates into set.
     *        It is required that the CHIP certificates in the chipCerts buffer stays valid while
     *        the certificates data in the set is used.
     *        In case of an error the certificate set is left in the same state as prior to this call.
     *
     * @param chipCerts     Buffer containing array of certificates or a single certificate encoded in CHIP TLV form.
     * @param chipCertsLen  The length of the certificates buffer.
     * @param decodeFlags   Certificate decoding option flags.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR LoadCerts(const uint8_t * chipCerts, uint32_t chipCertsLen, BitFlags<CertDecodeFlags> decodeFlags);

    /**
     * @brief Load CHIP certificates into set.
     *        It is required that the CHIP certificates in the reader's underlying buffer stays valid while
     *        the certificates data in the set is used.
     *        In case of an error the certificate set is left in the same state as prior to this call.
     *
     * @param reader       A TLVReader positioned at the CHIP certificates TLV array
     *                     or TLV structure for a single certificate.
     * @param decodeFlags  Certificate decoding option flags.
     *
     * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR LoadCerts(chip::TLV::TLVReader & reader, BitFlags<CertDecodeFlags> decodeFlags);

    /**
     * @brief Find certificate in the set.
     *
     * @param subjectKeyId  Subject key identifier of the certificate to be found in the set.
     *
     * @return A pointer to the certificate data On success. Otherwise, NULL if no certificate found.
     **/
    const ChipCertificateData * FindCert(const CertificateKeyId & subjectKeyId) const;

    /**
     * @return A pointer to the set of certificate data entries.
     **/
    const ChipCertificateData * GetCertSet() const { return mCerts; }

    /**
     * @return A pointer to the last certificate data in the set. Returns NULL if certificate set is empty.
     **/
    const ChipCertificateData * GetLastCert() const { return (mCertCount > 0) ? &mCerts[mCertCount - 1] : nullptr; }

    /**
     * @return Number of certificates loaded into the set.
     **/
    uint8_t GetCertCount() const { return mCertCount; }

    /**
     * @brief Check whether certificate is in the set.
     *
     * @param cert  Pointer to the ChipCertificateData structures.
     *
     * @return True if certificate is in the set, false otherwise.
     **/
    bool IsCertInTheSet(const ChipCertificateData * cert) const;

    /**
     * @brief Validate CHIP certificate.
     *
     * @param cert     Pointer to the CHIP certificate to be validated. The certificate is
     *                 required to be in this set, otherwise this function returns error.
     * @param context  Certificate validation context.
     *
     * @return Returns a CHIP_ERROR on validation or other error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR ValidateCert(const ChipCertificateData * cert, ValidationContext & context);

    /**
     * @brief Find and validate CHIP certificate.
     *
     * @param[in]  subjectDN     Subject distinguished name to use as certificate search parameter.
     * @param[in]  subjectKeyId  Subject key identifier to use as certificate search parameter.
     * @param[in]  context       Certificate validation context.
     * @param[out] certData      A slot to write a pointer to the CHIP certificate data that matches search criteria.
     *
     * @return Returns a CHIP_ERROR on validation or other error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR FindValidCert(const ChipDN & subjectDN, const CertificateKeyId & subjectKeyId, ValidationContext & context,
                             const ChipCertificateData ** certData);

    /**
     * @brief Verify CHIP certificate signature.
     *
     * @param cert    Pointer to the CHIP certificate which signature should be validated.
     * @param caCert  Pointer to the CA certificate of the verified certificate.
     *
     * @return Returns a CHIP_ERROR on validation or other error, CHIP_NO_ERROR otherwise
     **/
    static CHIP_ERROR VerifySignature(const ChipCertificateData * cert, const ChipCertificateData * caCert);

private:
    ChipCertificateData * mCerts; /**< Pointer to an array of certificate data. */
    uint8_t mCertCount;           /**< Number of certificates in mCerts
                                     array. We maintain the invariant that all
                                     the slots at indices less than
                                     mCertCount have been constructed and slots
                                     at indices >= mCertCount have either never
                                     had their constructor called, or have had
                                     their destructor called since then. */
    uint8_t mMaxCerts;            /**< Length of mCerts array. */
    bool mMemoryAllocInternal;    /**< Indicates whether temporary memory buffers are allocated internally. */

    /**
     * @brief Find and validate CHIP certificate.
     *
     * @param[in]  subjectDN      Subject distinguished name to use as certificate search parameter.
     * @param[in]  subjectKeyId   Subject key identifier to use as certificate search parameter.
     * @param[in]  context        Certificate validation context.
     * @param[in]  validateFlags  Certificate validation flags.
     * @param[in]  depth          Depth of the current certificate in the certificate validation chain.
     * @param[out] certData       A slot to write a pointer to the CHIP certificate data that matches search criteria.
     *
     * @return Returns a CHIP_ERROR on validation or other error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR FindValidCert(const ChipDN & subjectDN, const CertificateKeyId & subjectKeyId, ValidationContext & context,
                             BitFlags<CertValidateFlags> validateFlags, uint8_t depth, const ChipCertificateData ** certData);

    /**
     * @brief Validate CHIP certificate.
     *
     * @param cert           Pointer to the CHIP certificate to be validated.
     * @param context        Certificate validation context.
     * @param validateFlags  Certificate validation flags.
     * @param depth          Depth of the current certificate in the certificate validation chain.
     *
     * @return Returns a CHIP_ERROR on validation or other error, CHIP_NO_ERROR otherwise
     **/
    CHIP_ERROR ValidateCert(const ChipCertificateData * cert, ValidationContext & context,
                            BitFlags<CertValidateFlags> validateFlags, uint8_t depth);
};

/**
 * @brief Decode CHIP certificate.
 *        It is required that the CHIP certificate in the chipCert buffer stays valid while
 *        the certData is used.
 *
 * @param chipCert     Buffer containing CHIP certificate.
 * @param chipCertLen  The length of the CHIP certificate.
 * @param certData     Structure containing data extracted from the CHIP certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR DecodeChipCert(const uint8_t * chipCert, uint32_t chipCertLen, ChipCertificateData & certData);

/**
 * @brief Decode CHIP certificate.
 *        It is required that the CHIP certificate in the reader's underlying buffer stays valid while
 *        the certData is used.
 *
 * @param reader    A TLVReader positioned at the CHIP certificate TLV structure.
 * @param certData  Structure containing data extracted from the CHIP certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR DecodeChipCert(chip::TLV::TLVReader & reader, ChipCertificateData & certData);

/**
 * @brief Decode CHIP Distinguished Name (DN).
 *        It is required that the CHIP DN in the reader's underlying buffer stays valid while
 *        the dn structure is used.
 *
 * @param reader  A TLVReader positioned at the CHIP DN TLV structure.
 * @param dn      Distinguished Name structure.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR DecodeChipDN(chip::TLV::TLVReader & reader, ChipDN & dn);

/**
 * @brief Convert standard X.509 certificate to CHIP certificate.
 *
 * @param x509Cert        CHIP X.509 DER encoded certificate.
 * @param chipCertBuf     Buffer to store converted certificate in CHIP format.
 * @param chipCertBufSize The size of the buffer to store converted certificate.
 * @param chipCertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ConvertX509CertToChipCert(const ByteSpan x509Cert, uint8_t * chipCertBuf, uint32_t chipCertBufSize,
                                     uint32_t & chipCertLen);

/**
 * @brief Convert standard X.509 certificates to CHIP certificate array.
 *        This function takes NOC, and ICA cert X.509 certificates and
 *        encodes into CHIP certificate array.
 *
 *        NOC certificate must be provided.
 *        ICA certificate is optional. It can be omitted by providing a 0 length ByteSpan.
 *
 *        The API enforces that the NOC is issued by ICA (if ICA is provided).
 *
 * @param x509NOC           Node operational credentials certificate in X.509 DER encoding.
 * @param x509ICAC          Intermediate CA certificate in X.509 DER encoding.
 * @param chipCertArray     Buffer to store converted certificates in CHIP format.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ConvertX509CertsToChipCertArray(const ByteSpan & x509NOC, const ByteSpan & x509ICAC, MutableByteSpan & chipCertArray);

/**
 * @brief Extract NOC and ICA Certificates from a CHIP certificate array.
 *        This function takes a CHIP certificate array and splits it into single
 *        NOC and ICA CHIP certificates.
 *
 *        NOC certificate must be provided.
 *        ICA certificate is optional. It will be omitted (nullptr, 0) if the CHIP certificate array contains only one entry.
 *
 * @param opCertArray Chip certificate array.
 * @param noc[out]    Node operational credentials certificate in CHIP format.
 * @param icac[out]   Intermediate CA certificate in CHIP format.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ExtractCertsFromCertArray(const ByteSpan & opCertArray, ByteSpan & noc, ByteSpan & icac);

/**
 * @brief Convert CHIP certificate to the standard X.509 DER encoded certificate.
 *
 * @param chipCert        CHIP certificate in CHIP TLV encoding.
 * @param x509CertBuf     Buffer to store converted certificate in X.509 DER format.
 * @param x509CertBufSize The size of the buffer to store converted certificate.
 * @param x509CertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR ConvertChipCertToX509Cert(const ByteSpan chipCert, uint8_t * x509CertBuf, uint32_t x509CertBufSize,
                                     uint32_t & x509CertLen);

/**
 * @brief Generate a standard X.509 DER encoded certificate using provided CHIP certificate and signing key
 *
 * @param chipCert        Buffer containing CHIP certificate.
 * @param chipCertLen     The length of the CHIP certificate.
 * @param keypair         The certificate signing key
 * @param x509CertBuf     Buffer to store signed certificate in X.509 DER format.
 * @param x509CertBufSize The size of the buffer to store converted certificate.
 * @param x509CertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR GenerateSignedX509CertFromChipCert(const uint8_t * chipCert, uint32_t chipCertLen, Crypto::P256Keypair & keypair,
                                              uint8_t * x509CertBuf, uint32_t x509CertBufSize, uint32_t & x509CertLen);

// TODO: Add support for Authentication Tag Attribute
struct X509CertRequestParams
{
    int64_t SerialNumber;
    uint64_t Issuer;
    uint32_t ValidityStart;
    uint32_t ValidityEnd;
    bool HasFabricID;
    uint64_t FabricID;
    bool HasNodeID;
    uint64_t NodeID;
};

enum CertificateIssuerLevel
{
    kIssuerIsRootCA,
    kIssuerIsIntermediateCA,
};

/**
 * @brief Generate a new X.509 DER encoded Root CA certificate
 *
 * @param requestParams   Certificate request parameters.
 * @param issuerKeypair   The certificate signing key
 * @param x509CertBuf     Buffer to store signed certificate in X.509 DER format.
 * @param x509CertBufSize The size of the buffer to store converted certificate.
 * @param x509CertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR NewRootX509Cert(const X509CertRequestParams & requestParams, Crypto::P256Keypair & issuerKeypair, uint8_t * x509CertBuf,
                           uint32_t x509CertBufSize, uint32_t & x509CertLen);

/**
 * @brief Generate a new X.509 DER encoded Intermediate CA certificate
 *
 * @param requestParams   Certificate request parameters.
 * @param subject         The requested subject ID
 * @param subjectPubkey   The public key of subject
 * @param issuerKeypair   The certificate signing key
 * @param x509CertBuf     Buffer to store signed certificate in X.509 DER format.
 * @param x509CertBufSize The size of the buffer to store converted certificate.
 * @param x509CertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR NewICAX509Cert(const X509CertRequestParams & requestParams, uint64_t subject,
                          const Crypto::P256PublicKey & subjectPubkey, Crypto::P256Keypair & issuerKeypair, uint8_t * x509CertBuf,
                          uint32_t x509CertBufSize, uint32_t & x509CertLen);

/**
 * @brief Generate a new X.509 DER encoded Node operational certificate
 *
 * @param requestParams   Certificate request parameters.
 * @param issuerLevel     Indicates if the issuer is a root CA or an intermediate CA
 * @param subjectPubkey   The public key of subject
 * @param issuerKeypair   The certificate signing key
 * @param x509CertBuf     Buffer to store signed certificate in X.509 DER format.
 * @param x509CertBufSize The size of the buffer to store converted certificate.
 * @param x509CertLen     The length of the converted certificate.
 *
 * @return Returns a CHIP_ERROR on error, CHIP_NO_ERROR otherwise
 **/
CHIP_ERROR NewNodeOperationalX509Cert(const X509CertRequestParams & requestParams, CertificateIssuerLevel issuerLevel,
                                      const Crypto::P256PublicKey & subjectPubkey, Crypto::P256Keypair & issuerKeypair,
                                      uint8_t * x509CertBuf, uint32_t x509CertBufSize, uint32_t & x509CertLen);

/**
 * @brief
 *   Convert a certificate date/time (in the form of an ASN.1 universal time structure) into a CHIP Epoch UTC time.
 *
 * @note
 *   This function makes no attempt to verify the correct range of the input time other than year.
 *   Therefore callers must make sure the supplied values are valid prior to invocation.
 *
 * @param asn1Time   The calendar date/time to be converted.
 * @param epochTime  A reference to an integer that will receive CHIP Epoch UTC time.
 *
 * @retval  #CHIP_NO_ERROR                      If the input time was successfully converted.
 * @retval  #ASN1_ERROR_UNSUPPORTED_ENCODING    If the input time contained a year value that could not
 *                                              be represented in a CHIP epoch UTC time value.
 **/
CHIP_ERROR ASN1ToChipEpochTime(const chip::ASN1::ASN1UniversalTime & asn1Time, uint32_t & epochTime);

/**
 * @brief
 *   Convert a CHIP epoch UTC time into an ASN.1 universal time structure.
 *
 * @param epochTime  A CHIP epoch UTC time to be converted.
 * @param asn1Time   A reference to an ASN1UniversalTime structure to receive the date/time.
 *
 * @retval  #CHIP_NO_ERROR                      If the input time was successfully converted.
 */
CHIP_ERROR ChipEpochToASN1Time(uint32_t epochTime, chip::ASN1::ASN1UniversalTime & asn1Time);

/**
 *  @return  True if the OID represents a CHIP-defined 64-bit distinguished named attribute.
 **/
inline bool IsChip64bitDNAttr(chip::ASN1::OID oid)
{
    return (oid == chip::ASN1::kOID_AttributeType_ChipNodeId || oid == chip::ASN1::kOID_AttributeType_ChipFirmwareSigningId ||
            oid == chip::ASN1::kOID_AttributeType_ChipICAId || oid == chip::ASN1::kOID_AttributeType_ChipRootId ||
            oid == chip::ASN1::kOID_AttributeType_ChipFabricId);
}

/**
 *  @return  True if the OID represents a CHIP-defined 32-bit distinguished named attribute.
 **/
inline bool IsChip32bitDNAttr(chip::ASN1::OID oid)
{
    return (oid == chip::ASN1::kOID_AttributeType_ChipAuthTag1 || oid == chip::ASN1::kOID_AttributeType_ChipAuthTag2);
}

/**
 *  @return  True if the OID represents a CHIP-defined distinguished named attribute.
 **/
inline bool IsChipDNAttr(chip::ASN1::OID oid)
{
    return (IsChip64bitDNAttr(oid) || IsChip32bitDNAttr(oid));
}

/**
 * @brief Convert an ASN.1 DER encoded integer to a raw big-endian integer.
 *
 * @param derInt     P256 integer in ASN.1 DER encoded form.
 * @param rawInt     Buffer to store converted raw integer.
 * @param rawIntLen  The length of the converted raw integer.
 *
 * @retval  #CHIP_NO_ERROR  If the integer value was successfully converted.
 */
CHIP_ERROR ConvertIntegerDERToRaw(ByteSpan derInt, uint8_t * rawInt, const uint16_t rawIntLen);

/**
 * @brief Convert a raw CHIP signature to an ASN.1 DER encoded signature structure.
 *
 * @param rawSig        P256 ECDSA signature in raw form.
 * @param derSig        Buffer to store converted ASN.1 DER encoded signature.
 * @param derSigBufSize The size of the buffer to store ASN.1 DER encoded signature.
 * @param derSigLen     The length of the ASN.1 DER encoded signature.
 *
 * @retval  #CHIP_NO_ERROR  If the signature value was successfully converted.
 */
CHIP_ERROR ConvertECDSASignatureRawToDER(P256ECDSASignatureSpan rawSig, uint8_t * derSig, const uint16_t derSigBufSize,
                                         uint16_t & derSigLen);

/**
 * @brief Convert a raw CHIP ECDSA signature to an ASN.1 DER encoded signature structure.
 *
 * @param rawSig        P256 ECDSA signature in raw form.
 * @param writer        A reference to the ASN1Writer to store ASN.1 DER encoded signature.
 *
 * @retval  #CHIP_NO_ERROR  If the signature value was successfully converted.
 */
CHIP_ERROR ConvertECDSASignatureRawToDER(P256ECDSASignatureSpan rawSig, ASN1::ASN1Writer & writer);

/**
 * @brief Convert an ASN.1 DER encoded ECDSA signature to a raw CHIP signature.
 *
 * @param reader        A reference to the ASN1Reader positioned at the beginning of the
 *                      DER encoded ECDSA signature.
 * @param writer        A reference to the TLVWriter to store TLV encoded ECDSA signature element.
 * @param tag           Tag to use for TLV encoded signature.
 *
 * @retval  #CHIP_NO_ERROR  If the signature value was successfully converted.
 */
CHIP_ERROR ConvertECDSASignatureDERToRaw(ASN1::ASN1Reader & reader, chip::TLV::TLVWriter & writer, uint64_t tag);

/**
 * Extract a PeerId from an operational certificate that has already been
 * parsed.
 *
 * @return CHIP_ERROR_INVALID_ARGUMENT if the passed-in cert does not have at
 * least one NodeId RDN and one FabricId RDN in the Subject DN.  No other
 * validation (e.g. checkign that there is exactly one RDN of each type) is
 * performed.
 */
CHIP_ERROR ExtractPeerIdFromOpCert(const ChipCertificateData & opcert, PeerId * peerId);

/**
 * Extract a PeerId from an operational certificate in ByteSpan TLV-encoded
 * form.  This does not perform any sort of validation on the certificate
 * structure other than parsing it.
 *
 * Can return any error that can be returned from parsing the cert or from the
 * ChipCertificateData* version of ExtractPeerIdFromOpCert.
 */
CHIP_ERROR ExtractPeerIdFromOpCert(const ByteSpan & opcert, PeerId * peerId);

/**
 * Extract a PeerId from an operational certificate array in ByteSpan
 * TLV-encoded form.  This does not perform any sort of validation on the
 * certificate structure other than parsing it.
 *
 * Can return any error that can be returned from parsing the array or from the
 * ChipCertificateData* version of ExtractPeerIdFromOpCert.
 */
CHIP_ERROR ExtractPeerIdFromOpCertArray(const ByteSpan & opcertarray, PeerId * peerId);

} // namespace Credentials
} // namespace chip
